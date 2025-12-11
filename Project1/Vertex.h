#pragma once
#include <DirectXMath.h>
#include <cstdint>  // для std::uint16_t

using namespace DirectX;

// Структуры вершин ИЗ СЛАЙДОВ
struct Vertex1
{
    XMFLOAT3 Pos;      // 0-byte offset
    XMFLOAT4 Color;    // 12-byte offset
};

struct Vertex2
{
    XMFLOAT3 Pos;      // 0-byte offset
    XMFLOAT3 Normal;   // 12-byte offset
    XMFLOAT2 Tex0;     // 24-byte offset
    XMFLOAT2 Tex1;     // 32-byte offset
};

// Для примера со слайда будем использовать Vertex1 (с цветом)
using Vertex = Vertex1;

// Данные куба ИЗ СЛАЙДА (8 вершин)
const Vertex cubeVertices[] =
{
    { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },  // White
    { XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) },  // Black
    { XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },  // Red
    { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },  // Green
    { XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },  // Blue
    { XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) },  // Yellow
    { XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },  // Cyan
    { XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) }   // Magenta
};

const int cubeVertexCount = 8;

// Индексы для куба ИЗ СЛАЙДА (36 индексов = 12 треугольников * 3 вершины)
const std::uint16_t cubeIndices[] = {
    // front face (передняя грань)
    0, 1, 2,
    0, 2, 3,
    // back face (задняя грань)
    4, 6, 5,
    4, 7, 6,
    // left face (левая грань)
    4, 5, 1,
    4, 1, 0,
    // right face (правая грань)
    3, 2, 6,
    3, 6, 7,
    // top face (верхняя грань)
    1, 5, 6,
    1, 6, 2,
    // bottom face (нижняя грань)
    4, 0, 3,
    4, 3, 7
};

const int cubeIndexCount = 36;