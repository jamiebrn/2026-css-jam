#include "util.h"

void DrawSlantedQuad(float x, float y, float w, float h, float slant, Color color) {
    Vector2 v1 = { x + slant, y };
    Vector2 v2 = { x, y + h };
    Vector2 v3 = { x + w + slant, y };
    Vector2 v4 = { x + w, y + h };

    DrawTriangle(v1, v2, v3, color);
    DrawTriangle(v3, v2, v4, color);
}
