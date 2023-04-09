#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <cassert>

#ifndef EMSCRIPTEN
#include <skia/include/core/SkPath.h>
#include <skia/include/core/SkPaint.h>
#include <skia/include/core/SkMatrix.h>
class SkCanvas;
#else
#include <emscripten.h>
#endif

#include <Client/Ui/Element.hh>
#include <Client/Ui/Container.hh>

namespace app
{
    class Renderer;
    class Mouse;
}

extern app::Renderer *g_Renderer;
extern app::Mouse *g_Mouse;
namespace app
{
    class Renderer;
    class Mouse
    {
    public:
        float m_MouseX = 0.0f;
        float m_MouseY = 0.0f;
        uint8_t m_MouseState = 0;
        uint8_t m_MouseButton = 0;
        std::map<uint8_t, uint8_t> m_KeysPressed{};
        Mouse()
        {
            g_Mouse = this;
        }
    };
    class Guard
    {
        float m_CurrentMatrix[9];
        Renderer *m_Renderer;

    public:
        Guard(Renderer *);
        Guard(Renderer const &) = delete;
        Guard(Renderer &&) = delete;
        Renderer &operator=(Renderer const &) = delete;
        Renderer &operator=(Renderer &&) = delete;
        ~Guard();
    };
    // template <typename T = >
    class Renderer
    {
        using T = void (*)();
#ifndef EMSCRIPTEN
        SkCanvas *m_Canvas;
        SkPath m_CurrentPath;
        SkPaint m_FillPaint;
        SkPaint m_StrokePaint;
#endif
        float m_Matrix[9] = {
            1, 0, 0,  // s s x !!REMEMBER: CANVAS2D IS s s s s x y SO ORDER MATTERS ITS 0 1 3 4 2 5
            0, 1, 0,  // s s y
            0, 0, 1}; // 0 0 1

        friend class Guard;

        // state
        void Save();
        void Restore();

    public:

        float m_Width;
        float m_Height;
        float m_WindowScale;
        uint8_t m_ContextId = 0;
        ui::Container m_Container;

        enum class LineCap
        {
            Butt,
            Round,
            Square
        };
        enum class LineJoin
        {
            Bevel,
            Miter,
            Round
        };
        enum class TextAlign
        {
            Left,
            Center,
            Right
        };
        enum class TextBaseline
        {
            Top,
            Middle,
            Bottom
        };

        Renderer()
            : m_Container(*this)
        {
#ifndef EMSCRIPTEN
            m_StrokePaint.setAntiAlias(true);
            m_FillPaint.setAntiAlias(true);
            g_Renderer = this;
#else
            m_ContextId = EM_ASM_INT({ return Module.addCtx(); });
            if (m_ContextId == 0) {
                assert(!g_Renderer);
                g_Renderer = this;
            }
#endif
        }

        void SetSize(int32_t width, int32_t height);

        // matrix
        float const *GetTransform();
        void ResetTransform();
        void UpdateTransform();
        void SetTransform(float, float, float, float, float, float);
        void Rotate(float);
        void Translate(float, float);
        void Scale(float, float);

        // style
        void SetFill(uint32_t);
        void SetStroke(uint32_t);
        void SetLineWidth(float);
        void SetLineCap(LineCap);
        void SetLineJoin(LineJoin);
        void SetTextSize(float);
        void SetTextAlign(TextAlign);
        void SetTextBaseline(TextBaseline);
        void SetGlobalAlpha(float);

        // path
        void BeginPath();
        void MoveTo(float, float);
        void LineTo(float, float);
        void QuadraticCurveTo(float, float, float, float);
        void Arc(float, float, float);
        void FillRect(float, float, float, float);
        void StrokeRect(float, float, float, float);
        void Rect(float, float, float, float);
        void RoundRect(float, float, float, float, float);
        void FillText(std::string const &, float, float);
        void StrokeText(std::string const &, float, float);
        void Clip();

        float GetTextLength(std::string const &);

        // render
        void Stroke();
        void Fill();
    };

    class Path2D
    {
#ifdef EMSCRIPTEN
        uint32_t m_Index;
#else
        SkPath m_Path;
#endif

    public:
        Path2D();
        ~Path2D();

        void MoveTo(float, float);
        void LineTo(float, float);
        void ArcTo(float, float, float);
        void QuadTo(float, float, float, float);
    };
}

#ifdef EMSCRIPTEN
#else
void GlfwKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
        g_Renderer->m_KeysPressed[key] = 1;
    else if (action == GLFW_RELEASE)
        g_Renderer->m_KeysPressed[key] = 0;
}
#endif
