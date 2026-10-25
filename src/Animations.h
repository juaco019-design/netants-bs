#pragma once
// Animaciones - Recreacion de la famosa animacion de hormigas de NetAnts
// Original: AVI en recursos (ID 131) con hormigas llevando hojas
// Nueva: array de bitmaps generados proceduralmente + wxTimer

#include <wx/wx.h>
#include <vector>

class AntAnimation {
public:
    // Genera 8 frames de hormiga caminando con carga
    static std::vector<wxBitmap> GenerateFrames(int size = 32) {
        std::vector<wxBitmap> frames;
        for (int f = 0; f < 8; ++f) {
            wxImage img(size, size);
            img.InitAlpha();
            img.Clear(); // transparente
            // Dibuja hormiga simple pixel art animada
            // Cuerpo
            int cx = size/2 + (f % 2 == 0 ? 1 : -1); // bamboleo
            int cy = size/2;
            // Cabeza
            img.SetRGB(cx-6, cy-4, 139,69,19);
            img.SetRGB(cx-5, cy-4, 139,69,19);
            // Torax
            img.SetRGB(cx-2, cy-2, 160,82,45);
            img.SetRGB(cx-1, cy-2, 160,82,45);
            img.SetRGB(cx-2, cy-1, 160,82,45);
            // Abdomen
            img.SetRGB(cx+2, cy, 101,67,33);
            img.SetRGB(cx+3, cy, 101,67,33);
            // Patas animadas
            int legOffset = (f < 4) ? f : 7 - f;
            img.SetRGB(cx-3, cy+2+legOffset/2, 0,0,0);
            img.SetRGB(cx,   cy+2-legOffset/2, 0,0,0);
            img.SetRGB(cx+3, cy+2+legOffset/2, 0,0,0);
            // Hoja verde que lleva
            img.SetRGB(cx-4, cy-8, 0, 170 + f*5, 0);
            img.SetRGB(cx-3, cy-8, 0, 200, 0);
            img.SetRGB(cx-2, cy-8, 85, 255, 85);
            // Sombra
            img.SetRGB(cx, size-2, 200,200,200);
            
            wxBitmap bmp(img);
            frames.push_back(bmp);
        }
        return frames;
    }

    // Progress bar con hormigas (reemplaza la barra clasica)
    static void DrawAntProgress(wxDC& dc, wxRect rect, double fraction) {
        // fraction 0..1
        dc.SetBrush(wxBrush(wxColour(220,220,220)));
        dc.SetPen(wxPen(wxColour(180,180,180)));
        dc.DrawRectangle(rect);
        if (fraction <= 0) return;
        wxRect fill = rect;
        fill.width = int(rect.width * fraction);
        dc.SetBrush(wxBrush(wxColour(0,180,0)));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(fill);
        // hormiguita en el frente
        int ax = fill.GetRight() - 6;
        int ay = rect.y + rect.height/2;
        dc.SetPen(wxPen(*wxBLACK,1));
        dc.SetBrush(wxBrush(wxColour(139,69,19)));
        dc.DrawCircle(ax, ay, 4);
    }
};
