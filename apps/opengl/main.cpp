#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <array>
#include <string>
#include <algorithm>
#include <cmath>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "vao.h"
#include "vbo.h"
#include "ebo.h"
#include "shaderClass.h"

#define STB_EASY_FONT_IMPLEMENTATION
#include "stb_easy_font.h"

struct EasyV { float x, y, z; unsigned char color[4]; };

/* ── Constants ───────────────────────────────────────────────────────────── */

static constexpr float TWO_PI = 6.28318530718f;

inline constexpr int EURO_WHEEL_ORDER[37] = {
    0,32,15,19,4,21,2,25,17,34,6,27,13,36,11,30,8,23,10,
    5,24,16,33,1,20,14,31,9,22,18,29,7,28,12,35,3,26
};

inline constexpr int AMER_WHEEL_ORDER[38] = {
    0,28,9,26,30,11,7,20,32,17,5,22,34,15,3,24,36,13,1,
    0,27,10,25,29,12,8,19,31,18,6,21,33,16,4,23,35,14,2
};

const float PI = 3.14159265358979323846f;

// spin state vars
static float gAngle = 0.0f;      
static float gOmega = 0.0f;      
static float gAlpha = 0.0f;      // angular decel
static double gLastT = 0.0;      // last frame time
static bool gSpinning = false;



/* ── Helpers ─────────────────────────────────────────────────────────────── */

static std::vector<std::string> makeLabels(bool american) {
    const int* order = american ? AMER_WHEEL_ORDER : EURO_WHEEL_ORDER;
    int N = american ? 38 : 37;

    std::vector<std::string> L;
    L.reserve(N);

    int zero_seen = 0;
    for (int i = 0; i < N; ++i) {
        L.push_back(american && order[i] == 0
                        ? (zero_seen++ ? "00" : "0")
                        : std::to_string(order[i]));
    }
    return L;
}



/* ── GL Objects ──────────────────────────────────────────────────────────── */

VAO *wheelVAO = nullptr, *circleVAO = nullptr, *textVAO = nullptr;
VBO *wheelVBO = nullptr,  *circleVBO = nullptr,  *textVBO = nullptr;
EBO *wheelEBO = nullptr,  *circleEBO = nullptr;

GLsizei wheelIndexCount = 0;
GLsizei textVertexCount  = 0;



/* ── Callbacks ───────────────────────────────────────────────────────────── */

static void framebuffer_size_callback(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
}



/* ── Outer Wood Ring ─────────────────────────────────────────────────────── */

static void SetupOuterCircle(float r = 0.5f, int segs = 98) {
    std::vector<float>   v   ((segs + 2) * 3);
    std::vector<unsigned> idx (segs + 2);

    v[0] = v[1] = v[2] = 0.0f;

    for (int i = 0; i < segs; ++i) {
        float t = (TWO_PI * i) / segs;
        int j   = (i + 1) * 3;

        v[j + 0] = r * cosf(t);
        v[j + 1] = r * sinf(t);
        v[j + 2] = 0.0f;
    }

    int j = (segs + 1) * 3;
    v[j + 0] = v[3];
    v[j + 1] = v[4];
    v[j + 2] = v[5];

    idx[0] = 0;
    for (int i = 0; i <= segs; ++i) idx[i + 1] = i + 1;

    delete circleVAO; delete circleVBO; delete circleEBO;

    circleVAO = new VAO();
    circleVBO = new VBO(v.data(), v.size() * sizeof(float));
    circleEBO = new EBO(idx.data(), idx.size() * sizeof(unsigned));

    circleVAO->Bind();
    circleVBO->Bind();
    circleEBO->Bind();

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glDisableVertexAttribArray(1);
    glVertexAttrib3f(1, 0.36f, 0.22f, 0.07f);

    circleVAO->Unbind();
    circleVBO->Unbind();
    circleEBO->Unbind();
}

static void DrawOuterCircle(int segs = 98) {
    circleVAO->Bind();
    glDrawElements(GL_TRIANGLE_FAN, segs + 2, GL_UNSIGNED_INT, 0);
    circleVAO->Unbind();
}



/* ── Colored Wedges (Wheel Ring) ─────────────────────────────────────────── */

static void SetupWheelRing(const std::vector<std::string>& labels,
                           float rIn, float rOut) {
    int   N  = (int)labels.size();
    float dθ = TWO_PI / N;

    std::vector<float>    verts; verts.reserve(N * 4 * 6);
    std::vector<unsigned> inds;  inds.reserve(N * 6);

    auto colorFor = [&](int i) -> std::array<float, 3> {
        if (!labels[i].empty() && labels[i][0] == '0') return {0.30f, 0.50f, 0.30f};
        return (i % 2 == 0) ? std::array<float, 3>{0.70f, 0.20f, 0.20f}
                            : std::array<float, 3>{0.15f, 0.15f, 0.15f};
    };

    for (int i = 0; i < N; ++i) {
        float t0 = i * dθ;
        float t1 = t0 + dθ;

        float x0i = rIn  * cosf(t0), y0i = rIn  * sinf(t0);
        float x0o = rOut * cosf(t0), y0o = rOut * sinf(t0);
        float x1o = rOut * cosf(t1), y1o = rOut * sinf(t1);
        float x1i = rIn  * cosf(t1), y1i = rIn  * sinf(t1);

        auto c = colorFor(i);
        unsigned base = (unsigned)(verts.size() / 6);

        verts.insert(verts.end(), {
            x0i, y0i, 0,  c[0], c[1], c[2],
            x0o, y0o, 0,  c[0], c[1], c[2],
            x1o, y1o, 0,  c[0], c[1], c[2],
            x1i, y1i, 0,  c[0], c[1], c[2]
        });

        inds.insert(inds.end(), { base, base + 1, base + 2,
                                  base, base + 2, base + 3 });
    }

    delete wheelVAO; delete wheelVBO; delete wheelEBO;

    wheelVAO = new VAO();
    wheelVBO = new VBO(verts.data(), verts.size() * sizeof(float));
    wheelEBO = new EBO(inds.data(), inds.size() * sizeof(unsigned));
    wheelIndexCount = (GLsizei)inds.size();

    wheelVAO->Bind();
    wheelVBO->Bind();
    wheelEBO->Bind();

    GLsizei stride = 6 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    wheelVAO->Unbind();
    wheelVBO->Unbind();
    wheelEBO->Unbind();
}

static void DrawWheelRing() {
    wheelVAO->Bind();
    glDrawElements(GL_TRIANGLES, wheelIndexCount, GL_UNSIGNED_INT, 0);
    wheelVAO->Unbind();
}



/* ── Text (Numbers) ──────────────────────────────────────────────────────── */

static void BuildTextMesh(const std::vector<std::string>& labels,
                          float /*rIn*/, float rOut,
                          float scale = 0.0023f,
                          float rotation_offset = 0.0f)
{
    delete textVAO; delete textVBO;

    const int   N   = (int)labels.size();
    const float dθ  = TWO_PI / N;
    const float rT  = rOut * 0.96f;

    std::vector<float> verts; verts.reserve(N * 6 * 40); // pos(3)+color(3)
    char buf[20000];

    for (int i = 0; i < N; ++i) {
        const float θ  = i * dθ + dθ * 0.5f + rotation_offset;
        const float px = rT * cosf(θ);
        const float py = rT * sinf(θ);

        // Fill buf with quads for this label; q = number of quads
        int q = stb_easy_font_print(
            0.0f, 0.0f,
            const_cast<char*>(labels[i].c_str()),
            nullptr,                      // color per-vertex not needed
            buf, sizeof(buf)
        );

        // Interpret buf correctly
        auto* v = reinterpret_cast<const EasyV*>(buf);

        // Center the text
        float minx=1e9f,miny=1e9f,maxx=-1e9f,maxy=-1e9f;
        for (int vi = 0; vi < q * 4; ++vi) {
            minx = std::min(minx, v[vi].x); maxx = std::max(maxx, v[vi].x);
            miny = std::min(miny, v[vi].y); maxy = std::max(maxy, v[vi].y);
        }
        const float cx = 0.5f * (minx + maxx);
        const float cy = 0.5f * (miny + maxy);

        // Each quad -> two triangles
        static const int tri[6] = {0,1,2, 0,2,3};

        for (int qi = 0; qi < q; ++qi) {
            const int base = qi * 4;
            for (int t = 0; t < 6; ++t) {
                const EasyV& vv = v[base + tri[t]];
                const float x = (vv.x - cx) * scale;
                const float y = -(vv.y - cy) * scale;   // flip Y (stb is y-down)

                float φ = θ - 0.5f * PI;
                float xr = x * cosf(φ) - y * sinf(φ);
                float yr = x * sinf(φ) + y * cosf(φ);

                verts.insert(verts.end(), {
                    xr + px, yr + py, 0.001f,   // position
                    1.0f, 1.0f, 1.0f          // color (white)
                });
            }
        }
    }

    textVertexCount = (GLsizei)(verts.size() / 6);

    textVAO = new VAO();
    textVBO = new VBO(verts.data(), verts.size() * sizeof(float));
    textVAO->Bind(); textVBO->Bind();
    const GLsizei stride = 6 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    textVAO->Unbind(); textVBO->Unbind();
}


static void DrawText() {
    if (!textVAO) return;

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    textVAO->Bind();
    glDrawArrays(GL_TRIANGLES, 0, textVertexCount);
    textVAO->Unbind();

    glDisable(GL_BLEND);
}



/* ── App ─────────────────────────────────────────────────────────────────── */

int main() {
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 800, "OpenGL Window", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    // imgui for ui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    Shader wheelShader("opengl/shaders/wheel.vert", "opengl/shaders/wheel.frag");

    const float rIn  = 0.22f;
    const float rOut = 0.42f;

    bool american = false;
    auto labels   = makeLabels(american);

    SetupOuterCircle();
    SetupWheelRing(labels, rIn, rOut);
    BuildTextMesh(labels, rIn, rOut);

    bool tPrev = false;



    while (!glfwWindowShouldClose(window)) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int fbw, fbh; glfwGetFramebufferSize(window, &fbw, &fbh);

        const ImGuiViewport* vp = ImGui::GetMainViewport();   // reliable points, not pixels
        ImVec2 pos  = ImVec2(vp->WorkPos.x, vp->WorkPos.y + vp->WorkSize.y - 64.0f);
        ImVec2 size = ImVec2(vp->WorkSize.x, 64.0f);

        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowSize(size);
        ImGui::Begin("Controls", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);

        ImGui::SetCursorPosY(16);
        if (ImGui::Button("Spin Wheel", ImVec2(160, 32)) && !gSpinning) {
            // kick off a spin
            gSpinning = true;
            gAngle = fmodf(gAngle, TWO_PI);
            gOmega = 10.0f;      // initial speed (rad/s) – tweak to taste
            gAlpha = -2.0f;      // constant decel (rad/s^2)
        }
        ImGui::SameLine();
        ImGui::Text("Angle: %.2f rad   Speed: %.2f rad/s", gAngle, gOmega);
        ImGui::End();

        bool tNow = glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS;
        if (tNow && !tPrev) {
            american = !american;
            labels   = makeLabels(american);
            SetupWheelRing(labels, rIn, rOut);
            BuildTextMesh(labels, rIn, rOut);
        }
        tPrev = tNow;

        glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        wheelShader.Activate();

        // outer circle doesn't rotate
        glUniform1f(glGetUniformLocation(wheelShader.ID, "uAngle"), 0.0f);
        DrawOuterCircle();

        glUniform1f(glGetUniformLocation(wheelShader.ID, "uAngle"), gAngle);
        DrawWheelRing();
        DrawText();

        glfwPollEvents();

        double now = glfwGetTime();
        if (gLastT == 0.0) gLastT = now;
        double dt = now - gLastT;
        gLastT = now;

        if (gSpinning) {
            gOmega += gAlpha * (float)dt;
            if (gOmega <= 0.0f) { gOmega = 0.0f; gSpinning = false; }
            gAngle += gOmega * (float)dt;
            if (gAngle > TWO_PI) gAngle = fmodf(gAngle, TWO_PI);
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    wheelShader.Delete();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
