#include "Terrain.h"

#include <wavefront.h>
#include <image_io.h>

Terrain::Terrain(const TerrainSpecs& specs)
    : m_Specs(specs)
{
    loadData();
    initChunks();
}

Terrain::Terrain()
    : Terrain(TerrainSpecs())
{}

Terrain::~Terrain()
{
}

void Terrain::loadData()
{
    m_HeightMap = read_image("data/terrain/terrain.png");

    auto imageData = read_image_data("data/CubeWorld/Blocks_PixelArt.png");
    ASSERT(imageData.size > 0, "Could not load data/CubeWorld/Blocks_PixelArt.png");
    ASSERT(m_SpriteSheetTexture.loadFromImage(imageData), "Could not load spritesheet from image data");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    ASSERT(m_CubeShader.load("data/shaders/TP_SI3D/Cube.glsl"), "Could not load data/shaders/TP_SI3D/Cube.glsl");

    m_Meshes.emplace_back(read_indexed_mesh("data/CubeWorld/Pixel Blocks/OBJ/Dirt.obj"));
    m_Meshes.emplace_back(read_indexed_mesh("data/CubeWorld/Pixel Blocks/OBJ/Grass.obj"));
    
    for (const auto& mesh : m_Meshes)
        m_MultiMesh.addMesh(mesh);

    m_MultiMesh.createBuffers();
}

void Terrain::initChunks()
{
    constexpr float CUBE_INITIAL_SIZE = 2.f;

    m_ChunkManager.init(m_Specs.chunkX, m_Specs.chunkY, m_Specs.chunkWidth, m_HeightMap, m_Specs.cubesHeight, CUBE_INITIAL_SIZE, m_Specs.cubeSize);

    m_SSBO.generate();
    m_SSBO.setBindingPoint(0);
    std::cout << "Init ssbo data size: " << m_Specs.chunkX * m_Specs.chunkY * m_Specs.chunkWidth * m_Specs.chunkWidth << " * " << sizeof(Transform)
        << " = " << m_Specs.chunkX * m_Specs.chunkY * m_Specs.chunkWidth * m_Specs.chunkWidth * sizeof(Transform) << "B \n";
    m_SSBO.setData(nullptr, m_Specs.chunkX * m_Specs.chunkY * m_Specs.chunkWidth * m_Specs.chunkWidth * sizeof(Transform));

    for (size_t i = 0; i < m_ChunkManager.getChunks().size(); ++i)
    {
        auto& chunk = m_ChunkManager.getChunks().at(i);
        auto offset = m_ChunkManager.getChunkFirstInstanceIndice().at(i);
        m_SSBO.setSubData(chunk.getInstanceTransforms().data(), chunk.getInstanceTransforms().size() * sizeof(Transform), offset * sizeof(Transform));

        // std::cout << "SSBO subdata, offset: " << offset << ", data size: " << chunk.getInstanceTransforms().size() << "\n";

        m_MultiMesh.addCommand(0, chunk.getInstanceCount(), offset);
    }
    m_MultiMesh.updateCommandsBuffer();
}

void Terrain::draw(const Transform& view, const Transform& projection) const
{
    m_CubeShader.bind();
    m_CubeShader.setUniform("viewMatrix", view);
    m_CubeShader.setUniform("projectionMatrix", projection);
    m_CubeShader.setTextureUniform(m_SpriteSheetTexture, 0, "spriteSheet");

    m_MultiMesh.draw();
}

void Terrain::cullChunks(const Transform& view, const Transform& projection)
{
    const Transform viewProj = projection * view;

    #if LOG_SHOWN_CHUNKS
    size_t shownChunks = 0;
    #endif

    for (size_t i = 0; i < m_ChunkManager.getChunks().size(); ++i)
    {
        const auto& chunk = m_ChunkManager.getChunks().at(i);
        auto offset = m_ChunkManager.getChunkFirstInstanceIndice().at(i);

        if (AabbCrossesFrustum(chunk.getboundingBox(), ViewFrustum(viewProj)))
        {
            m_MultiMesh.setCommand(i, 0, chunk.getInstanceCount(), offset);
            #if LOG_SHOWN_CHUNKS
            ++shownChunks;
            #endif
        }
        else
        {
            m_MultiMesh.setCommand(i, 0, 0, offset);
        }
    }

    m_MultiMesh.updateCommandsBuffer();

    #if LOG_SHOWN_CHUNKS
    if (shownChunks != m_LastShownChunks)
    {
        std::cout << "Drawing " << shownChunks << "/" << m_ChunkManager.getChunks().size() << " chunks\n";
        m_LastShownChunks = shownChunks;
    }
    #endif
}

void Terrain::stopCulling()
{
    #if LOG_SHOWN_CHUNKS
    m_LastShownChunks = 0;
    size_t shownChunks = 0;
    #endif

    for (size_t i = 0; i < m_ChunkManager.getChunks().size(); ++i)
    {
        const auto& chunk = m_ChunkManager.getChunks().at(i);
        auto offset = m_ChunkManager.getChunkFirstInstanceIndice().at(i);

        m_MultiMesh.setCommand(i, 0, chunk.getInstanceCount(), offset);
        #if LOG_SHOWN_CHUNKS
        ++shownChunks;
        #endif
    }

    m_MultiMesh.updateCommandsBuffer();

    #if LOG_SHOWN_CHUNKS
    std::cout << "Drawing " << shownChunks << "/" << m_ChunkManager.getChunks().size() << " chunks\n";
    #endif
}