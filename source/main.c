#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include "vshader_shbin.h"

#define CLEAR_COLOR 0x7898B8FF

#define DISPLAY_TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
     GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
     GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

typedef struct {
    float x, y, z;
    float r, g, b;
} Vertex;

#define V(x,y,z,r,g,b) {x,y,z,r,g,b}

/* Camera-space geometry: the camera looks toward negative Z. */
static const Vertex vertices[] = {
    /* Asphalt road */
    V(-3.0f,-1.35f,-2.0f, .15f,.16f,.18f),
    V( 3.0f,-1.35f,-2.0f, .15f,.16f,.18f),
    V( 3.0f,-1.35f,-30.0f,.15f,.16f,.18f),
    V(-3.0f,-1.35f,-2.0f, .15f,.16f,.18f),
    V( 3.0f,-1.35f,-30.0f,.15f,.16f,.18f),
    V(-3.0f,-1.35f,-30.0f,.15f,.16f,.18f),

    /* Left sidewalk */
    V(-4.2f,-1.32f,-2.0f, .55f,.52f,.45f),
    V(-3.0f,-1.32f,-2.0f, .55f,.52f,.45f),
    V(-3.0f,-1.32f,-30.0f, .55f,.52f,.45f),
    V(-4.2f,-1.32f,-2.0f, .55f,.52f,.45f),
    V(-3.0f,-1.32f,-30.0f, .55f,.52f,.45f),
    V(-4.2f,-1.32f,-30.0f, .55f,.52f,.45f),

    /* Right sidewalk */
    V(3.0f,-1.32f,-2.0f, .59f,.56f,.49f),
    V(4.2f,-1.32f,-2.0f, .59f,.56f,.49f),
    V(4.2f,-1.32f,-30.0f, .59f,.56f,.49f),
    V(3.0f,-1.32f,-2.0f, .59f,.56f,.49f),
    V(4.2f,-1.32f,-30.0f, .59f,.56f,.49f),
    V(3.0f,-1.32f,-30.0f, .59f,.56f,.49f),

    /* Dashed yellow road markings */
    V(-.07f,-1.30f,-4.0f, .95f,.76f,.25f),
    V( .07f,-1.30f,-4.0f, .95f,.76f,.25f),
    V( .07f,-1.30f,-5.0f, .95f,.76f,.25f),
    V(-.07f,-1.30f,-4.0f, .95f,.76f,.25f),
    V( .07f,-1.30f,-5.0f, .95f,.76f,.25f),
    V(-.07f,-1.30f,-5.0f, .95f,.76f,.25f),

    V(-.07f,-1.30f,-7.0f, .95f,.76f,.25f),
    V( .07f,-1.30f,-7.0f, .95f,.76f,.25f),
    V( .07f,-1.30f,-8.0f, .95f,.76f,.25f),
    V(-.07f,-1.30f,-7.0f, .95f,.76f,.25f),
    V( .07f,-1.30f,-8.0f, .95f,.76f,.25f),
    V(-.07f,-1.30f,-8.0f, .95f,.76f,.25f),

    V(-.07f,-1.30f,-10.0f, .95f,.76f,.25f),
    V( .07f,-1.30f,-10.0f, .95f,.76f,.25f),
    V( .07f,-1.30f,-11.0f, .95f,.76f,.25f),
    V(-.07f,-1.30f,-10.0f, .95f,.76f,.25f),
    V( .07f,-1.30f,-11.0f, .95f,.76f,.25f),
    V(-.07f,-1.30f,-11.0f, .95f,.76f,.25f),
};

/* Append a colored rectangular building block to a static mesh. */
static Vertex city[36 * 8];
static int city_count;

static void quad(Vertex a, Vertex b, Vertex c, Vertex d,
                 float r, float g, float bl)
{
    if (city_count + 6 > (int)(sizeof(city) / sizeof(city[0]))) return;
    a.r=b.r=c.r=d.r=r;
    a.g=b.g=c.g=d.g=g;
    a.b=b.b=c.b=d.b=bl;
    city[city_count++] = a;
    city[city_count++] = b;
    city[city_count++] = c;
    city[city_count++] = a;
    city[city_count++] = c;
    city[city_count++] = d;
}

static void building(float x1, float x2, float yTop,
                     float zFront, float zBack,
                     float r, float g, float b)
{
    float base = -1.30f;
    /* Front face */
    quad(V(x1,base,zFront,0,0,0), V(x2,base,zFront,0,0,0),
         V(x2,yTop,zFront,0,0,0), V(x1,yTop,zFront,0,0,0), r,g,b);
    /* Side face */
    quad(V(x2,base,zFront,0,0,0), V(x2,base,zBack,0,0,0),
         V(x2,yTop,zBack,0,0,0), V(x2,yTop,zFront,0,0,0),
         r*.72f,g*.72f,b*.72f);
    /* Roof */
    quad(V(x1,yTop,zFront,0,0,0), V(x2,yTop,zFront,0,0,0),
         V(x2,yTop,zBack,0,0,0), V(x1,yTop,zBack,0,0,0),
         r*.85f,g*.85f,b*.85f);
}

static DVLB_s *shader_dvlb;
static shaderProgram_s program;
static int projection_loc;
static C3D_Mtx projection;
static void *vbo_ground;
static void *vbo_city;

static void sceneInit(void)
{
    shader_dvlb = DVLB_ParseFile((u32 *)vshader_shbin, vshader_shbin_size);
    shaderProgramInit(&program);
    shaderProgramSetVsh(&program, &shader_dvlb->DVLE[0]);
    C3D_BindProgram(&program);
    projection_loc = shaderInstanceGetUniformLocation(program.vertexShader, "projection");

    C3D_AttrInfo *attr = C3D_GetAttrInfo();
    AttrInfo_Init(attr);
    AttrInfo_AddLoader(attr, 0, GPU_FLOAT, 3);
    AttrInfo_AddLoader(attr, 1, GPU_FLOAT, 3);

    Mtx_PerspTilt(&projection, C3D_AngleFromDegrees(55.0f),
                  400.0f / 240.0f, 0.1f, 100.0f, true);

    /* A handful of simple block buildings on both sides of the road. */
    building(-4.0f,-2.8f,2.2f,-5.0f,-8.0f, .48f,.22f,.16f);
    building(-4.1f,-2.8f,3.3f,-9.0f,-13.0f, .37f,.40f,.43f);
    building(-4.0f,-2.8f,2.7f,-14.0f,-18.0f, .50f,.32f,.20f);
    building(-4.2f,-2.8f,4.0f,-19.0f,-24.0f, .35f,.38f,.44f);
    building(2.8f,4.0f,3.0f,-5.0f,-8.0f, .17f,.28f,.40f);
    building(2.8f,4.1f,2.3f,-9.0f,-13.0f, .57f,.41f,.26f);
    building(2.8f,4.0f,3.7f,-14.0f,-18.0f, .40f,.42f,.43f);
    building(2.8f,4.2f,2.6f,-19.0f,-24.0f, .46f,.22f,.17f);

    vbo_ground = linearAlloc(sizeof(vertices));
    memcpy(vbo_ground, vertices, sizeof(vertices));
    vbo_city = linearAlloc(sizeof(Vertex) * city_count);
    memcpy(vbo_city, city, sizeof(Vertex) * city_count);

    C3D_TexEnv *env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, 0, 0);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
}

static void drawMesh(void *vbo, int count)
{
    C3D_BufInfo *buf = C3D_GetBufInfo();
    BufInfo_Init(buf);
    BufInfo_Add(buf, vbo, sizeof(Vertex), 2, 0x0C);
    C3D_DrawArrays(GPU_TRIANGLES, 0, count);
}

static void sceneRender(void)
{
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, projection_loc, &projection);
    drawMesh(vbo_ground, (int)(sizeof(vertices) / sizeof(vertices[0])));
    drawMesh(vbo_city, city_count);
}

static void sceneExit(void)
{
    linearFree(vbo_ground);
    linearFree(vbo_city);
    shaderProgramFree(&program);
    DVLB_Free(shader_dvlb);
}

int main(void)
{
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

    C3D_RenderTarget *target = C3D_RenderTargetCreate(
        240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
    C3D_DepthTestEnable(GPU_GEQUAL, GPU_WRITE_ALL);

    sceneInit();
    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & KEY_START) break;

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(target, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
        C3D_FrameDrawOn(target);
        sceneRender();
        C3D_FrameEnd(0);
    }

    sceneExit();
    C3D_Fini();
    gfxExit();
    return 0;
}
