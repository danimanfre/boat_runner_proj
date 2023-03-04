// This has been adapted from the Vulkan tutorial
#include "header.hpp"
using namespace std;
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <optional>
#include <set>
#include <cstdint>
#include <algorithm>
#include <fstream>
#include <array>
#include <unordered_map>
//#include <unistd.h>
#include <math.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

// The uniform buffer object used in this example
struct globalUniformBufferObject {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::vec3 lightDir;
    alignas(16) glm::vec4 lightColor;
    alignas(16) glm::vec3 eyePos;
};

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

/*
    In this section we set up some parameteres which can be useful to guarantee
    the correct functioning of the game, as for example the boundaries of our game.
    It is also specified the number of rocks for our game and so it is possible to
    increase the difficulty of the game.
*/ 

const float x_coordinate = -5.0;

// Position of screen for the sea and for the gameover
static glm::vec3 global_pos_sea = glm::vec3(-1, 0, 0);

const float scale_boat = 0.001;

const float scale_boat_gold = 0.00025;

const float scale_mini_boat = 0.0005;

const float scale_rock1 = 0.04;

const float scale_rock2 = 0.1;

// Boat position and orientation
static glm::vec3 global_pos_boat = glm::vec3(x_coordinate / scale_boat, 0, 600);

// Position and orientation of gold boat
static glm::vec3 pos_gold_boat = glm::vec3(x_coordinate / scale_boat_gold, 3600, -7500 + rand() % 7500);
float boatYaw = 0.0;
float boatPitch = 0.0;
float boatRoll = 0.0;

float boat_rotation = 0.0f;

// boundaries of the game
static int upBound = 1000;
static int lowBound = -2500;
static int leftBound = 3000;
static int rightBound = -3000;

// boundaries for rock1 creation
static int upBoundRock1 = 30;
static int lowBoundRock1 = 25;
static int leftBoundRock1 = 80;
static int rightBoundRock1 = -70;

// boundaries for rock1 creation
static int upBoundRock2 = 12;
static int lowBoundRock2 = 9;
static int leftBoundRock2 = 40;
static int rightBoundRock2 = -35;

// vector of position for rock of type 1
vector<glm::vec3> pos_rock_1;

// vector of position for rock of type 2
vector<glm::vec3> pos_rock_2;

static glm::mat3 CamDir = glm::mat3(1.0f);
static glm::vec3 CamPos = glm::vec3(0.0f, 0.0f, 0.0f);

// enumeration for game status
enum gameState
{
    START, // before starting the game
    GAME,  // game going on
    COLLISION, // when a collision is detected
    GAME_OVER // game ended
};

// minumum number of rocks 
static const int minRockNum = 3;
// maximum number of rocks 
static const int maxRockNum = 5;

// game
int game_life = 3;

// actual game life
int actual_game_life = 3;

// score of the actual game
float score;

// when this variable is set to true means that the player has detected a gold boat
// and so his game life increase of 1
bool detectGoldCollision = false;

// This is the main section of our code
class MyProject : public BaseProject {
protected:
    // Here you list all the Vulkan objects you need:

    // Descriptor Layouts for standard boat and rocks[what will be passed to the shaders]
    DescriptorSetLayout DSLglobal;
    DescriptorSetLayout DSLglobal_boat;

    // Descriptor Layouts for boat collision
    DescriptorSetLayout DSLobj;
    DescriptorSetLayout DSLobj_boat;

    // Descriptor Layouts for image 
    DescriptorSetLayout DSLglobal_image;
    DescriptorSetLayout DSLobj_image;

    // Descriptor Layouts for gold boat 
    DescriptorSetLayout DSLglobal_gold_boat;
    DescriptorSetLayout DSLobj_gold_boat;

    // Pipelines [Shader couples]
    Pipeline P1;
    Pipeline P2;
    Pipeline P3;
    Pipeline P4;

    // Models, textures and Descriptors for the normal boat (values assigned to the uniforms)
    Model M_SlBoat;
    Texture T_SlBoat;
    DescriptorSet DS_SlBoat; // instance DSLobj

    // Models, textures and Descriptors for the boat when it is in collision mode (values assigned to the uniforms)
    Model M_SlBoat_coll;
    Texture T_SlBoat_coll;
    DescriptorSet DS_SlBoat_coll;

    // Models, textures and Descriptors for the gold boat
    Model M_SlBoat_gold;
    Texture T_SlBoat_gold;
    DescriptorSet DS_SlBoat_gold;

    // Models, textures and Descriptors for the boats used as counter of remaining lives
    std::vector <Model> M_boat_vec;
    std::vector <Texture> T_boat_vec;
    std::vector <DescriptorSet> DS_boat_vec;

    // Models, textures and Descriptors vectors for the rock of type 2
    std::vector <Model> M_SlRock2;
    std::vector <Texture> T_SlRock2;
    std::vector <DescriptorSet> DS_SlRock2;

    // Models, textures and Descriptors vectors for the rock of type 1
    std::vector <Model> M_SlRock1;
    std::vector <Texture> T_SlRock1;
    std::vector <DescriptorSet> DS_SlRock1;

    // Models, textures and Descriptors for the sea image
    Model M_SlSea;
    Texture T_SlSea;
    DescriptorSet DS_SlSea;

    // Models, textures and Descriptors for the start image
    Model M_SlStartImage;
    Texture T_SlStartImage;
    DescriptorSet DS_SlStartImage;

    // Models, textures and Descriptors for the gameover image
    Model M_SlGameOverImage;
    Texture T_SlGameOverImage;
    DescriptorSet DS_SlGameOverImage;

    // variable for the actual game status 
    gameState actual_state;

    // random generation of number of rocks. Integer between minRockNum and maxRockNum 
    int rockCount = rand() % (maxRockNum - minRockNum + 1) + minRockNum;

    

    
    // Global descriptors set
    DescriptorSet DS_global;
    DescriptorSet DS_global_coll;
    DescriptorSet DS_global_image;
    DescriptorSet DS_global_gold_boat;

    // Here you set the main application parameters
    void setWindowParameters() {
        // window size, titile and initial background
        windowWidth = 1280;
        windowHeight = 916;
        windowTitle = "Boat Runner. Your best score is: " + std::to_string(readBestScore("best_score.txt"));
        initialBackgroundColor = { 0.529f, 0.808f, 0.922f, 1.0f };


        // Descriptor pool sizes
        uniformBlocksInPool = 2*rockCount + 10 + game_life;
        texturesInPool = 2*rockCount + 9 + game_life;
        setsInPool = 2*rockCount + 10 + game_life;

    }

    // Here you load and setup all your Vulkan objects
    void localInit() {
        
        // initialiting score
        score = 0.0f;

        // set the intial status of the game
        actual_state = START;

        // generate in a random position the first type of rocks
        for (int i = 0; i < rockCount; i++) {
            pos_rock_1.push_back(glm::vec3(x_coordinate / scale_rock1, lowBoundRock1 + rand() % (upBoundRock1 - lowBoundRock1), rightBoundRock1 + rand() % (leftBoundRock1 - rightBoundRock1)));
        }

        // generate in a random position the second type of rocks
        for (int i = 0; i < rockCount; i++) {
            pos_rock_2.push_back(glm::vec3(x_coordinate / scale_rock2, lowBoundRock2 + rand() % (upBoundRock2 - lowBoundRock2), rightBoundRock2 + rand() % (leftBoundRock2 - rightBoundRock2)));
        }

        // Descriptor Layouts [what will be passed to the shaders]
        DSLobj.init(this, {
            // this array contains the binding:
            // first  element : the binding number
            // second element : the time of element (buffer or texture)
            // third  element : the pipeline stage where it will be used
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
            });

        DSLobj_boat.init(this, {
            // this array contains the binding:
            // first  element : the binding number
            // second element : the time of element (buffer or texture)
            // third  element : the pipeline stage where it will be used
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
            });


        

        DSLobj_image.init(this, {
            // this array contains the binding:
            // first  element : the binding number
            // second element : the time of element (buffer or texture)
            // third  element : the pipeline stage where it will be used
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
            });


        DSLobj_gold_boat.init(this, {
            // this array contains the binding:
            // first  element : the binding number
            // second element : the time of element (buffer or texture)
            // third  element : the pipeline stage where it will be used
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
            });

        DSLglobal.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
            });

        DSLglobal_boat.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
            });

        DSLglobal_image.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
            });

        DSLglobal_gold_boat.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
            });

        

        // Pipelines [Shader couples]
        // The last array, is a vector of pointer to the layouts of the sets that will
        // be used in this pipeline. The first element will be set 0, and so on..
        P1.init(this, "shaders/vert.spv", "shaders/frag.spv", { &DSLglobal, &DSLobj });
        P2.init(this, "shaders/vert.spv", "shaders/frag_boat.spv", { &DSLglobal_boat, &DSLobj_boat });
        P3.init(this, "shaders/vert.spv", "shaders/frag_image.spv", { &DSLglobal_image, &DSLobj_image });
        P4.init(this, "shaders/vert.spv", "shaders/frag_gold_boat.spv", { &DSLglobal_gold_boat, &DSLobj_gold_boat });

        M_SlBoat.init(this, "C:/Users/dmanf/source/repos/Boat_runner_proj/Asset_Boat_Runner/Boat/Boat.obj");
        T_SlBoat.init(this, "C:/Users/dmanf/source/repos/Boat_runner_proj/Asset_Boat_Runner/Boat/textures/boat_diffuse.bmp");
        DS_SlBoat.init(this, &DSLobj, {
            {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
            {1, TEXTURE, 0, &T_SlBoat}
            });

        M_SlSea.init(this, "C:/Users/dmanf/source/repos/Boat_runner_proj/Asset_Boat_Runner/Boat/sea2.obj");
        T_SlSea.init(this, "C:/Users/dmanf/source/repos/Boat_runner_proj/Asset_Boat_Runner/Boat/textures/sea.jpg");
        DS_SlSea.init(this, &DSLobj_image, {
            {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
            {1, TEXTURE, 0, &T_SlSea}
            });

        M_SlStartImage.init(this, "C:/Users/dmanf/source/repos/Boat_runner_proj/Asset_Boat_Runner/Boat/start_image.obj");
        T_SlStartImage.init(this, "C:/Users/dmanf/source/repos/Boat_runner_proj/Asset_Boat_Runner/Boat/textures/start.jpg");
        DS_SlStartImage.init(this, &DSLobj_image, {
            {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
            {1, TEXTURE, 0, &T_SlStartImage}
            });
        M_SlGameOverImage.init(this, "C:/Users/dmanf/source/repos/Boat_runner_proj/Asset_Boat_Runner/Boat/game_over.obj");
        T_SlGameOverImage.init(this, "C:/Users/dmanf/source/repos/Boat_runner_proj/Asset_Boat_Runner/Boat/textures/game_over.jpg");
        DS_SlGameOverImage.init(this, &DSLobj_image, {
            {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
            {1, TEXTURE, 0, &T_SlGameOverImage}
            });
        

        M_SlBoat_coll.init(this, "C:/Users/dmanf/source/repos/Boat_runner_proj/Asset_Boat_Runner/Boat/Boat.obj");
        T_SlBoat_coll.init(this, "C:/Users/dmanf/source/repos/Boat_runner_proj/Asset_Boat_Runner/Boat/textures/boat_diffuse.bmp");
        DS_SlBoat_coll.init(this, &DSLobj_boat, {
            {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
            {1, TEXTURE, 0, &T_SlBoat_coll}
            });

        M_SlBoat_gold.init(this, "C:/Users/dmanf/source/repos/Boat_runner_proj/Asset_Boat_Runner/Boat/Boat.obj");
        T_SlBoat_gold.init(this, "C:/Users/dmanf/source/repos/Boat_runner_proj/Asset_Boat_Runner/Boat/textures/boat_diffuse.bmp");
        DS_SlBoat_gold.init(this, &DSLobj_gold_boat, {
            {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
            {1, TEXTURE, 0, &T_SlBoat_gold}
            });

        
        for (int i = 0; i < rockCount; i++) {
            Model modelRock2;
            Texture tsRock2;
            DescriptorSet dsRock2;
            modelRock2.init(this, "C:/Users/dmanf/source/repos/Boat_runner_proj/Asset_Boat_Runner/Rocks/Rock2/Rock_1.obj");
            tsRock2.init(this, "C:/Users/dmanf/source/repos/Boat_runner_proj/Asset_Boat_Runner/Rocks/Rock2/Rock_1_Tex/Rock_1_Base_Color.jpg");
            dsRock2.init(this, &DSLobj, {
                {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
                {1, TEXTURE, 0, &tsRock2}
                });
            M_SlRock2.push_back(modelRock2);
            T_SlRock2.push_back(tsRock2);
            DS_SlRock2.push_back(dsRock2);

        }



        for (int i = 0; i < rockCount; i++) {
            Model modelRock;
            Texture tsRock;
            DescriptorSet dsRock;
            modelRock.init(this, "C:/Users/dmanf/source/repos/Boat_runner_proj/Asset_Boat_Runner/Rocks/Rock1/rock1.obj");
            tsRock.init(this, "C:/Users/dmanf/source/repos/Boat_runner_proj/Asset_Boat_Runner/Rocks/Rock1/textures/rock_low_Base_Color.png");
            dsRock.init(this, &DSLobj, {
                {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
                {1, TEXTURE, 0, &tsRock}
                });
            M_SlRock1.push_back(modelRock);
            T_SlRock1.push_back(tsRock);
            DS_SlRock1.push_back(dsRock);

        }

        for (int i = 0; i < game_life; i++) {
            Model modelBoat;
            Texture tBoat;
            DescriptorSet dsBoat;
            modelBoat.init(this, "C:/Users/dmanf/source/repos/Boat_runner_proj/Asset_Boat_Runner/Boat/Boat.obj");
            tBoat.init(this, "C:/Users/dmanf/source/repos/Boat_runner_proj/Asset_Boat_Runner/Boat/textures/boat_diffuse.bmp");
            dsBoat.init(this, &DSLobj, {
                {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
                {1, TEXTURE, 0, &tBoat}
                });
            M_boat_vec.push_back(modelBoat);
            T_boat_vec.push_back(tBoat);
            DS_boat_vec.push_back(dsBoat);

        }
        DS_global.init(this, &DSLglobal, {
            {0, UNIFORM, sizeof(globalUniformBufferObject), nullptr}
            });

        DS_global_coll.init(this, &DSLglobal_boat, {
            {0, UNIFORM, sizeof(globalUniformBufferObject), nullptr}
            });

        DS_global_image.init(this, &DSLglobal_image, {
            {0, UNIFORM, sizeof(globalUniformBufferObject), nullptr}
            });

        DS_global_gold_boat.init(this, &DSLglobal_gold_boat, {
            {0, UNIFORM, sizeof(globalUniformBufferObject), nullptr}
            });

    }

    // Here we destroy all the objects created
    void localCleanup() {
        DS_SlBoat.cleanup();
        T_SlBoat.cleanup();
        M_SlBoat.cleanup();

        DS_SlBoat_coll.cleanup();
        T_SlBoat_coll.cleanup();
        M_SlBoat_coll.cleanup();

        DS_SlBoat_gold.cleanup();
        T_SlBoat_gold.cleanup();
        M_SlBoat_gold.cleanup();

        DS_SlSea.cleanup();
        T_SlSea.cleanup();
        M_SlSea.cleanup();

        DS_SlStartImage.cleanup();
        T_SlStartImage.cleanup();
        M_SlStartImage.cleanup();
        

        DS_SlGameOverImage.cleanup();
        T_SlGameOverImage.cleanup();
        M_SlGameOverImage.cleanup();

        
        for (int i = 0; i < rockCount; i++) {
            DS_SlRock1[i].cleanup();
            T_SlRock1[i].cleanup();
            M_SlRock1[i].cleanup();
        }
        for (int i = 0; i < rockCount; i++) {
            DS_SlRock2[i].cleanup();
            T_SlRock2[i].cleanup();
            M_SlRock2[i].cleanup();
        }

        for (int i = 0; i < 3; i++) {
            M_boat_vec[i].cleanup();
            T_boat_vec[i].cleanup();
            DS_boat_vec[i].cleanup();
        }

        DS_global.cleanup();
        DS_global_coll.cleanup();
        DS_global_image.cleanup();
        DS_global_gold_boat.cleanup();
        

        P1.cleanup();
        P2.cleanup();
        P3.cleanup();
        P4.cleanup();

        

        DSLglobal.cleanup();
        DSLglobal_boat.cleanup();

        DSLobj.cleanup();
        DSLobj_boat.cleanup();

        DSLobj_image.cleanup();
        DSLglobal_image.cleanup();
        
        DSLobj_gold_boat.cleanup();
        DSLglobal_gold_boat.cleanup();
    }

    

    // Here it is the creation of the command buffer:
    // We send to the GPU all the objects you want to draw,
    // with their buffers and textures
    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
        
        /********** Pipeline 2 *************/
        
        std::cout << "Populate command buffer \n";
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            P2.graphicsPipeline);
        vkCmdBindDescriptorSets(commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            P2.pipelineLayout, 0, 1, &DS_global_coll.descriptorSets[currentImage],
            0, nullptr);
        
        

        VkBuffer vertexBuffers8[] = { M_SlBoat_coll.vertexBuffer };
        VkDeviceSize offsets8[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers8, offsets8);
        vkCmdBindIndexBuffer(commandBuffer, M_SlBoat_coll.indexBuffer, 0,
            VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            P2.pipelineLayout, 1, 1, &DS_SlBoat_coll.descriptorSets[currentImage],
            0, nullptr);
        vkCmdDrawIndexed(commandBuffer,
            static_cast<uint32_t>(M_SlBoat_coll.indices.size()), 1, 0, 0, 0);


        

        /********** Pipeline 3 *************/
        
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            P3.graphicsPipeline);
        vkCmdBindDescriptorSets(commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            P3.pipelineLayout, 0, 1, &DS_global_image.descriptorSets[currentImage],
            0, nullptr);


        VkBuffer vertexBuffers13[] = { M_SlSea.vertexBuffer };
        VkDeviceSize offsets13[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers13, offsets13);
        vkCmdBindIndexBuffer(commandBuffer, M_SlSea.indexBuffer, 0,
            VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            P3.pipelineLayout, 1, 1, &DS_SlSea.descriptorSets[currentImage],
            0, nullptr);
        vkCmdDrawIndexed(commandBuffer,
            static_cast<uint32_t>(M_SlSea.indices.size()), 1, 0, 0, 0);

        VkBuffer vertexBuffers14[] = { M_SlStartImage.vertexBuffer };
        VkDeviceSize offsets14[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers14, offsets14);
        vkCmdBindIndexBuffer(commandBuffer, M_SlStartImage.indexBuffer, 0,
            VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            P3.pipelineLayout, 1, 1, &DS_SlStartImage.descriptorSets[currentImage],
            0, nullptr);
        vkCmdDrawIndexed(commandBuffer,
            static_cast<uint32_t>(M_SlStartImage.indices.size()), 1, 0, 0, 0);

        VkBuffer vertexBuffers15[] = { M_SlGameOverImage.vertexBuffer };
        VkDeviceSize offsets15[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers15, offsets15);
        vkCmdBindIndexBuffer(commandBuffer, M_SlGameOverImage.indexBuffer, 0,
            VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            P3.pipelineLayout, 1, 1, &DS_SlGameOverImage.descriptorSets[currentImage],
            0, nullptr);
        vkCmdDrawIndexed(commandBuffer,
            static_cast<uint32_t>(M_SlGameOverImage.indices.size()), 1, 0, 0, 0);


        /********** Pipeline 4 *************/

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            P4.graphicsPipeline);
        vkCmdBindDescriptorSets(commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            P4.pipelineLayout, 0, 1, &DS_global_gold_boat.descriptorSets[currentImage],
            0, nullptr);
       
        VkBuffer vertexBuffers6[] = { M_SlBoat_gold.vertexBuffer };
        VkDeviceSize offsets6[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers6, offsets6);
        vkCmdBindIndexBuffer(commandBuffer, M_SlBoat_gold.indexBuffer, 0,
            VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            P4.pipelineLayout, 1, 1, &DS_SlBoat_gold.descriptorSets[currentImage],
            0, nullptr);
        vkCmdDrawIndexed(commandBuffer,
            static_cast<uint32_t>(M_SlBoat_gold.indices.size()), 1, 0, 0, 0);


        /********** Pipeline 1 *************/


        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            P1.graphicsPipeline);
        vkCmdBindDescriptorSets(commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            P1.pipelineLayout, 0, 1, &DS_global.descriptorSets[currentImage],
            0, nullptr);
        

        VkBuffer vertexBuffers10[] = { M_boat_vec[0].vertexBuffer };
        VkDeviceSize offsets10[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers10, offsets10);
        vkCmdBindIndexBuffer(commandBuffer, M_boat_vec[0].indexBuffer, 0,
            VK_INDEX_TYPE_UINT32);    

        for (int i = 0; i < game_life; i++)
        {
            vkCmdBindDescriptorSets(commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                P1.pipelineLayout, 1, 1, &DS_boat_vec[i].descriptorSets[currentImage],
                0, nullptr);
            vkCmdDrawIndexed(commandBuffer,
                static_cast<uint32_t>(M_boat_vec[0].indices.size()), 1, 0, 0, 0);
        }


        VkBuffer vertexBuffers3[] = { M_SlBoat.vertexBuffer };
        VkDeviceSize offsets3[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers3, offsets3);
        vkCmdBindIndexBuffer(commandBuffer, M_SlBoat.indexBuffer, 0,
            VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            P1.pipelineLayout, 1, 1, &DS_SlBoat.descriptorSets[currentImage],
            0, nullptr);
        vkCmdDrawIndexed(commandBuffer,
            static_cast<uint32_t>(M_SlBoat.indices.size()), 1, 0, 0, 0);


        VkBuffer vertexBuffers2[] = { M_SlRock2[0].vertexBuffer };
        VkDeviceSize offsets2[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers2, offsets2);
        vkCmdBindIndexBuffer(commandBuffer, M_SlRock2[0].indexBuffer, 0,
            VK_INDEX_TYPE_UINT32);
        for (int i = 0; i < rockCount; i++)
        {
            vkCmdBindDescriptorSets(commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                P1.pipelineLayout, 1, 1, &DS_SlRock2[i].descriptorSets[currentImage],
                0, nullptr);
            vkCmdDrawIndexed(commandBuffer,
                static_cast<uint32_t>(M_SlRock2[0].indices.size()), 1, 0, 0, 0);
        }
        

        VkBuffer vertexBuffers1[] = { M_SlRock1[0].vertexBuffer };
        VkDeviceSize offsets1[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers1, offsets1);
        vkCmdBindIndexBuffer(commandBuffer, M_SlRock1[0].indexBuffer, 0,
            VK_INDEX_TYPE_UINT32);
        for (int i = 0; i < rockCount; i++) {

            vkCmdBindDescriptorSets(commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                P1.pipelineLayout, 1, 1, &DS_SlRock1[i].descriptorSets[currentImage],
                0, nullptr);
            vkCmdDrawIndexed(commandBuffer,
                static_cast<uint32_t>(M_SlRock1[0].indices.size()), 1, 0, 0, 0);
        }



    }

    // Here is where you update the uniforms.
    // Very likely this will be where you will be writing the logic of your application.
    void updateUniformBuffer(uint32_t currentImage) {

        // set the start time of the game
        static auto startTime = std::chrono::high_resolution_clock::now();
        
        // set the current time
        auto currentTime = std::chrono::high_resolution_clock::now();

        // get the game time (from the start of the game and not from the start of application)
        float myTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        float game_time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        // initializing collision time
        static float collision_time = 0.0f;

        // initializing last time to compute the delta T
        static float lastTime = 0.0f;

        // initializing used to decrease the time to update the rocks position
        static float numberOfIteration = 10;

        //initializing the last time the player took a gold boat
        static float lastTimeBoatGold = 0.0f;
        
        // compute delta T
        float deltaT = myTime - lastTime;

        // compute delta T for gold boat
        float deltaTgold = myTime - lastTimeBoatGold;
        

        static struct CameraAngle {
            float x = 0.0f;
            float y = -90.0f;
            float z = 0.0f;
        } CamAngle;

        static struct CameraPosition {
            float x = -1.2f;
            float y = 0.0f;
            float z = -0.15f;
        } CamPos;


        const float MOVE_SPEED = 15.0;

        // left move
        if (glfwGetKey(window, GLFW_KEY_A)) {

            global_pos_boat += MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), 0.0f,
                glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1));
        }
        // right move
        if (glfwGetKey(window, GLFW_KEY_D)) {

            global_pos_boat -= MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), 0.0f,
                glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1));
                   
        }

        // down move
        if (glfwGetKey(window, GLFW_KEY_S)) {
            global_pos_boat -= MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), 0.0f,
                glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 1, 0, 1));
            boat_rotation = boat_rotation > 0.0f ? 0.0f: boat_rotation + 0.4f;
            
        }

        // up move
        if (glfwGetKey(window, GLFW_KEY_W)) {
            global_pos_boat += MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), 0.0f,
                glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 1, 0, 1));
            boat_rotation = boat_rotation < -20.0f ? -20.0f : boat_rotation - 0.4f;
        }

        // when the button space is pressed change the game status 
        if (glfwGetKey(window, GLFW_KEY_SPACE))
        {
            if (actual_state == START)
            {
                actual_state = GAME;
            }
            else if (actual_state == GAME_OVER)
            {
                restartGame();
            }
        }

        
        // when the game is going on we need to check the boundaries and update the score
        if (actual_state == GAME || actual_state == COLLISION)
        {
            checkBoatBoundaries();
            score += deltaT;

            // check if there is a collision with a gold boat 
            if (!detectGoldCollision && detectCollisionsGoldBoat(pos_gold_boat, global_pos_boat))
            {
                detectGoldCollision = true;
                lastTimeBoatGold = myTime;
                // update game life (the bound is game_life)
                actual_game_life = actual_game_life == game_life ? game_life : actual_game_life + 1;
            }
        }

        



        // business logic when game is going on
        if ((actual_state == GAME)) {
                
            // check collision with rock of type 1
            bool detectCollisioneType1 = detectCollisionsRock1(pos_rock_1, global_pos_boat);

            // check collision with rock of type 2
            bool detectCollisioneType2 = detectCollisionsRock2(pos_rock_2, global_pos_boat);
             
            // if any collision occur change game status in COLLISION and decrement life count.
            // Start the time because for 3 seconds boat is immune to any collision
            if (detectCollisioneType1 || detectCollisioneType2)
            {
                actual_state = COLLISION;
                collision_time = game_time;
                actual_game_life--;
                std::cout << "Life left :" << actual_game_life << "\n";
                    
                auto end = std::chrono::system_clock::now();

                std::time_t end_time = std::chrono::system_clock::to_time_t(end);

                    

                std::cout << "finished computation at " << std::ctime(&end_time);
                    
                std::cout << "Detect a collion at time: " << collision_time << "\n";


                // business logic when game ended: update history score and the best score    
                if (actual_game_life == 0)
                {
                    float actual_best_score = readBestScore("best_score.txt");
                    if (actual_best_score != 0)
                    {
                        std::cout << "Actual best score " << actual_best_score << "\n";
                    }
                    else 
                    {
                        // update window title with the new score
                        string title = "Boat Runner. Your best score is: " + std::to_string(score);
                        writeScore("best_score.txt", std::to_string(score));
                        glfwSetWindowTitle(window, title.c_str());
                    }

                    if (actual_best_score < score)
                    {
                        clearFile("best_score.txt");
                        writeScore("best_score.txt", std::to_string(score));
                    }
                        
                    std::cout << "Game over: score " << game_time << "\n";
                    string title = "Boat Runner. Your best score is: " + std::to_string(score);
                    writeScore("history_scores.txt", std::to_string(score) + " at " + std::ctime(&end_time) + "\n");
                    glfwSetWindowTitle(window, title.c_str());
                    // change state to game_over and then go to the game over interface
                    actual_state = GAME_OVER;

                }
            }
        }


        globalUniformBufferObject gubo{};
        UniformBufferObject ubo{};

        void* data;

        gubo.view = glm::rotate(glm::mat4(1.0f), glm::radians(CamAngle.x), glm::vec3(1, 0, 0)) *
            glm::rotate(glm::mat4(1.0f), glm::radians(CamAngle.y), glm::vec3(0, 1, 0)) *
            glm::rotate(glm::mat4(1.0f), glm::radians(CamAngle.z), glm::vec3(0, 0, 1)) *
            glm::translate(glm::mat4(1), glm::vec3(CamPos.x, CamPos.y, CamPos.z));

        gubo.proj = glm::perspective(glm::radians(45.0f),
            swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);

        gubo.proj[1][1] *= -1;
        

        
      


        vkMapMemory(device, DS_global.uniformBuffersMemory[0][currentImage], 0,
            sizeof(gubo), 0, &data);
        memcpy(data, &gubo, sizeof(gubo));
        vkUnmapMemory(device, DS_global.uniformBuffersMemory[0][currentImage]);

        vkMapMemory(device, DS_global_coll.uniformBuffersMemory[0][currentImage], 0,
            sizeof(gubo), 0, &data);
        memcpy(data, &gubo, sizeof(gubo));
        vkUnmapMemory(device, DS_global_coll.uniformBuffersMemory[0][currentImage]);

        vkMapMemory(device, DS_global_image.uniformBuffersMemory[0][currentImage], 0,
            sizeof(gubo), 0, &data);
        memcpy(data, &gubo, sizeof(gubo));
        vkUnmapMemory(device, DS_global_image.uniformBuffersMemory[0][currentImage]);

        vkMapMemory(device, DS_global_gold_boat.uniformBuffersMemory[0][currentImage], 0,
            sizeof(gubo), 0, &data);
        memcpy(data, &gubo, sizeof(gubo));
        vkUnmapMemory(device, DS_global_gold_boat.uniformBuffersMemory[0][currentImage]);
        

        // Uniform buffer objects
        UniformBufferObject ubo_boat{};
        UniformBufferObject ubo_boat_coll{};
        UniformBufferObject ubo_boat_gold{};
        UniformBufferObject ubo_boat_mini{};
        UniformBufferObject ubo_sea{};
        UniformBufferObject ubo_start_image{};
        UniformBufferObject ubo_game_over_image{};

        // switch to handle all the game status case. For each case the graphic is updated to show only relevant object
        switch (actual_state)
        {
        case START: {

            // start image
            ubo_start_image.model = glm::scale(glm::mat4(1.0f), glm::vec3(5.499f)) * glm::translate(glm::mat4(1.0f), global_pos_sea) * glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            vkMapMemory(device, DS_SlStartImage.uniformBuffersMemory[0][currentImage], 0,
                sizeof(ubo_start_image), 0, &data);
            memcpy(data, &ubo_start_image, sizeof(ubo_start_image));
            vkUnmapMemory(device, DS_SlStartImage.uniformBuffersMemory[0][currentImage]);
        }
            break;
        case GAME: {
            // boat model
            ubo_boat.model = glm::scale(glm::mat4(1.0f), glm::vec3(scale_boat)) * glm::translate(glm::mat4(1.0f), global_pos_boat) * glm::rotate(glm::mat4(1.0f), glm::radians(boat_rotation), glm::vec3(0.0f, 0.0f, 1.0f));
            vkMapMemory(device, DS_SlBoat.uniformBuffersMemory[0][currentImage], 0,
                sizeof(ubo_boat), 0, &data);
            memcpy(data, &ubo_boat, sizeof(ubo_boat));
            vkUnmapMemory(device, DS_SlBoat.uniformBuffersMemory[0][currentImage]);

            // hide the boat for collision
            ubo_boat_coll.model = glm::scale(glm::mat4(1.0f), glm::vec3(scale_boat)) * glm::translate(glm::mat4(1.0f), glm::vec3(10000, 10000, 10000)) * glm::rotate(glm::mat4(1.0f), glm::radians(boat_rotation), glm::vec3(0.0f, 0.0f, 1.0f));
            vkMapMemory(device, DS_SlBoat_coll.uniformBuffersMemory[0][currentImage], 0,
                sizeof(ubo_boat_coll), 0, &data);
            memcpy(data, &ubo_boat_coll, sizeof(ubo_boat_coll));
            vkUnmapMemory(device, DS_SlBoat_coll.uniformBuffersMemory[0][currentImage]);

            // hide the game over image when the game restart
            ubo_game_over_image.model = glm::scale(glm::mat4(1.0f), glm::vec3(20)) * glm::translate(glm::mat4(1.0f), glm::vec3(10000, 10000, 10000)) * glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            vkMapMemory(device, DS_SlGameOverImage.uniformBuffersMemory[0][currentImage], 0,
                sizeof(ubo_game_over_image), 0, &data);
            memcpy(data, &ubo_game_over_image, sizeof(ubo_game_over_image));
            vkUnmapMemory(device, DS_SlGameOverImage.uniformBuffersMemory[0][currentImage]);

        }
            break;
        case COLLISION: {

            // draw the boat collision model
            ubo_boat_coll.model = glm::scale(glm::mat4(1.0f), glm::vec3(scale_boat)) * glm::translate(glm::mat4(1.0f), global_pos_boat) * glm::rotate(glm::mat4(1.0f), glm::radians(boat_rotation), glm::vec3(0.0f, 0.0f, 1.0f));
            vkMapMemory(device, DS_SlBoat_coll.uniformBuffersMemory[0][currentImage], 0,
                sizeof(ubo_boat_coll), 0, &data);
            memcpy(data, &ubo_boat_coll, sizeof(ubo_boat_coll));
            vkUnmapMemory(device, DS_SlBoat_coll.uniformBuffersMemory[0][currentImage]);
            
            // hide the standard boat
            ubo_boat.model = glm::scale(glm::mat4(1.0f), glm::vec3(scale_boat)) * glm::translate(glm::mat4(1.0f), glm::vec3(10000, 10000, 10000)) * glm::rotate(glm::mat4(1.0f), glm::radians(.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            vkMapMemory(device, DS_SlBoat.uniformBuffersMemory[0][currentImage], 0,
                sizeof(ubo_boat), 0, &data);
            memcpy(data, &ubo_boat, sizeof(ubo_boat));
            vkUnmapMemory(device, DS_SlBoat.uniformBuffersMemory[0][currentImage]);


        }
            break;
        case GAME_OVER: {

            // show the game over image
            ubo_game_over_image.model = glm::scale(glm::mat4(1.0f), glm::vec3(5.45f)) * glm::translate(glm::mat4(1.0f), global_pos_sea) * glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            vkMapMemory(device, DS_SlGameOverImage.uniformBuffersMemory[0][currentImage], 0,
                sizeof(ubo_game_over_image), 0, &data);
            memcpy(data, &ubo_game_over_image, sizeof(ubo_game_over_image));
            vkUnmapMemory(device, DS_SlGameOverImage.uniformBuffersMemory[0][currentImage]);

            // hide gold boat model
            ubo_boat_gold.model = glm::scale(glm::mat4(1.0f), glm::vec3(0.0002)) * glm::translate(glm::mat4(1.0f), glm::vec3(-10000, 10000, 5200 ) -= (float)0.0 * glm::vec3(glm::rotate(glm::mat4(1.0f), 0.0f,
                glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 1, 0, 1))) * glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            vkMapMemory(device, DS_SlBoat_gold.uniformBuffersMemory[0][currentImage], 0,
                sizeof(ubo_boat_gold), 0, &data);
            memcpy(data, &ubo_boat_gold, sizeof(ubo_boat_gold));
            vkUnmapMemory(device, DS_SlBoat_gold.uniformBuffersMemory[0][currentImage]);

            
            for (int i = 0; i < rockCount; i++) {

                // hide all the rocks of type 2
                ubo.model = glm::scale(glm::mat4(1.0f), glm::vec3(scale_rock1)) * glm::translate(glm::mat4(1.0f), glm::vec3(1000, 1000, 10000) -= (float)0.15 * glm::vec3(glm::rotate(glm::mat4(1.0f), 0.0f,
                    glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 1, 0, 1))) * glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                //ubo.model = glm::scale(glm::mat4(1.0f), glm::vec3(0.0399)) * glm::translate(glm::mat4(1.0f), positions[i]) * glm::rotate(glm::mat4(1.0f), glm::radians(.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                vkMapMemory(device, DS_SlRock1[i].uniformBuffersMemory[0][currentImage], 0,
                    sizeof(ubo), 0, &data);
                memcpy(data, &ubo, sizeof(ubo));
                vkUnmapMemory(device, DS_SlRock1[i].uniformBuffersMemory[0][currentImage]);

                // hide all the rocks of type 2
                ubo.model = glm::scale(glm::mat4(1.0f), glm::vec3(scale_rock2)) * glm::translate(glm::mat4(1.0f), glm::vec3(1000, 1000, 10000) -= (float)0.00001 * score * glm::vec3(glm::rotate(glm::mat4(1.0f), 0.0f,
                    glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 1, 0, 1))) * glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                vkMapMemory(device, DS_SlRock2[i].uniformBuffersMemory[0][currentImage], 0,
                    sizeof(ubo), 0, &data);
                memcpy(data, &ubo, sizeof(ubo));
                vkUnmapMemory(device, DS_SlRock2[i].uniformBuffersMemory[0][currentImage]);

                
            }

            // hide the boat for collision
            ubo_boat_coll.model = glm::scale(glm::mat4(1.0f), glm::vec3(scale_boat)) * glm::translate(glm::mat4(1.0f), glm::vec3(10000, 1000, 10000)) * glm::rotate(glm::mat4(1.0f), glm::radians(.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            vkMapMemory(device, DS_SlBoat_coll.uniformBuffersMemory[0][currentImage], 0,
                sizeof(ubo_boat_coll), 0, &data);
            memcpy(data, &ubo_boat_coll, sizeof(ubo_boat_coll));
            vkUnmapMemory(device, DS_SlBoat_coll.uniformBuffersMemory[0][currentImage]);

            // hide the standard boat
            ubo_boat.model = glm::scale(glm::mat4(1.0f), glm::vec3(scale_boat)) * glm::translate(glm::mat4(1.0f), glm::vec3(10000, 10000, 10000)) * glm::rotate(glm::mat4(1.0f), glm::radians(.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            vkMapMemory(device, DS_SlBoat.uniformBuffersMemory[0][currentImage], 0,
                sizeof(ubo_boat), 0, &data);
            memcpy(data, &ubo_boat, sizeof(ubo_boat));
            vkUnmapMemory(device, DS_SlBoat.uniformBuffersMemory[0][currentImage]);
            
            // hide the game life boat
            for (int i = 0; i < game_life - actual_game_life; i++)
            {
                ubo_boat_mini.model = glm::scale(glm::mat4(1.0f), glm::vec3(scale_mini_boat)) * glm::translate(glm::mat4(1.0f), glm::vec3(-10000, 10000, 5200 + 200 * i)) * glm::rotate(glm::mat4(1.0f), glm::radians(.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                vkMapMemory(device, DS_boat_vec[i].uniformBuffersMemory[0][currentImage], 0,
                    sizeof(ubo_boat_mini), 0, &data);
                memcpy(data, &ubo_boat_mini, sizeof(ubo_boat_mini));
                vkUnmapMemory(device, DS_boat_vec[i].uniformBuffersMemory[0][currentImage]);
            }
        }
            break;
        }
        

        // common logic when game going on and the boat detect a collision
        if (actual_state == GAME || actual_state == COLLISION)
        {
            // draw the sea model
            ubo_sea.model = glm::scale(glm::mat4(1.0f), glm::vec3(5.49f)) * glm::translate(glm::mat4(1.0f), global_pos_sea) ;
            vkMapMemory(device, DS_SlSea.uniformBuffersMemory[0][currentImage], 0,
                sizeof(ubo_sea), 0, &data);
            memcpy(data, &ubo_sea, sizeof(ubo_sea));
            vkUnmapMemory(device, DS_SlSea.uniformBuffersMemory[0][currentImage]);
            
            // if a collision with a gold boat occurs this is not generated for the next 10 seconds
            if (detectGoldCollision)
            {
                ubo_boat_gold.model = glm::scale(glm::mat4(1.0f), glm::vec3(scale_boat_gold)) * glm::translate(glm::mat4(1.0f), glm::vec3(-10000, 10000, 5200) -= (float)0.0 * glm::vec3(glm::rotate(glm::mat4(1.0f), 0.0f,
                    glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 1, 0, 1))) * glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                vkMapMemory(device, DS_SlBoat_gold.uniformBuffersMemory[0][currentImage], 0,
                    sizeof(ubo_boat_gold), 0, &data);
                memcpy(data, &ubo_boat_gold, sizeof(ubo_boat_gold));
                vkUnmapMemory(device, DS_SlBoat_gold.uniformBuffersMemory[0][currentImage]);
            }
            else
            {
                ubo_boat_gold.model = glm::scale(glm::mat4(1.0f), glm::vec3(scale_boat_gold)) * glm::translate(glm::mat4(1.0f), pos_gold_boat -= (float)30.0 * glm::vec3(glm::rotate(glm::mat4(1.0f), 0.0f,
                    glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 1, 0, 1))) * glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                vkMapMemory(device, DS_SlBoat_gold.uniformBuffersMemory[0][currentImage], 0,
                    sizeof(ubo_boat_gold), 0, &data);
                memcpy(data, &ubo_boat_gold, sizeof(ubo_boat_gold));
                vkUnmapMemory(device, DS_SlBoat_gold.uniformBuffersMemory[0][currentImage]);
            }
            
            // draw the actual game life
            for (int i = 0; i < actual_game_life; i++)
            {
                ubo_boat_mini.model = glm::scale(glm::mat4(1.0f), glm::vec3(scale_mini_boat)) * glm::translate(glm::mat4(1.0f), glm::vec3(-10000, 3600, 5800 + 200 * i)) * glm::rotate(glm::mat4(1.0f), glm::radians(.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                vkMapMemory(device, DS_boat_vec[i].uniformBuffersMemory[0][currentImage], 0,
                    sizeof(ubo_boat_mini), 0, &data);
                memcpy(data, &ubo_boat_mini, sizeof(ubo_boat_mini));
                vkUnmapMemory(device, DS_boat_vec[i].uniformBuffersMemory[0][currentImage]);
            }

            // hide the lost game life
            for (int i = 0; i < game_life - actual_game_life; i++)
            {
                ubo_boat_mini.model = glm::scale(glm::mat4(1.0f), glm::vec3(scale_mini_boat)) * glm::translate(glm::mat4(1.0f), glm::vec3(-10000, 10000, 5200 + 200 * i)) * glm::rotate(glm::mat4(1.0f), glm::radians(.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                vkMapMemory(device, DS_boat_vec[i].uniformBuffersMemory[0][currentImage], 0,
                    sizeof(ubo_boat_mini), 0, &data);
                memcpy(data, &ubo_boat_mini, sizeof(ubo_boat_mini));
                vkUnmapMemory(device, DS_boat_vec[i].uniformBuffersMemory[0][currentImage]);
            }






            // Draw the rocks of type 2
            for (int i = 0; i < rockCount; i++) {

                float speed1 = 0.00001 * score < 1.10 ? 0.00001 * score : 1.10;
                ubo.model = glm::scale(glm::mat4(1.0f), glm::vec3(scale_rock2)) * glm::translate(glm::mat4(1.0f), pos_rock_2[i] -= speed1 * glm::vec3(glm::rotate(glm::mat4(1.0f), 0.0f,
                    glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 1, 0, 1))) * glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                vkMapMemory(device, DS_SlRock2[i].uniformBuffersMemory[0][currentImage], 0,
                    sizeof(ubo), 0, &data);
                memcpy(data, &ubo, sizeof(ubo));
                vkUnmapMemory(device, DS_SlRock2[i].uniformBuffersMemory[0][currentImage]);
            }


            // Draw the rocks of type 1
            for (int i = 0; i < rockCount; i++) {
                float speed1 = 0.00005 * score < 1.10 ? 0.00005 * score : 1.10;
                
                ubo.model = glm::scale(glm::mat4(1.0f), glm::vec3(scale_rock1)) * glm::translate(glm::mat4(1.0f), pos_rock_1[i] -= speed1 * glm::vec3(glm::rotate(glm::mat4(1.0f), 0.0f,
                    glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 1, 0, 1))) * glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                vkMapMemory(device, DS_SlRock1[i].uniformBuffersMemory[0][currentImage], 0,
                    sizeof(ubo), 0, &data);
                memcpy(data, &ubo, sizeof(ubo));
                vkUnmapMemory(device, DS_SlRock1[i].uniformBuffersMemory[0][currentImage]);

            }
        }

        // after detecting a COLLISION for the next 3 seconds the boat is immune to any collision with rocks
        if (actual_state == COLLISION && ((game_time - collision_time) > 3.0f))
        {
            std::cout << "collision time " << collision_time << "\n";
            std::cout << "game time " << game_time << "\n";
            actual_state = GAME;
            std::cout << "Fine tempo di collisione \n";
        }

        // when 10 seconds are elapsed we can generate a new boat gold
        if (deltaTgold > 10)
        {
            detectGoldCollision = false;
            pos_gold_boat = glm::vec3(x_coordinate / scale_boat_gold, 3600, -7500 + rand() % 15000);
            lastTimeBoatGold = myTime;
        }


        // when tot seconds are elapsed we can generate new rocks
        if (deltaT > numberOfIteration) {
            pos_rock_1.clear();
            pos_rock_2.clear();
            for (int i = 0; i < rockCount; i++) {
                pos_rock_1.push_back(glm::vec3(x_coordinate / scale_rock1, lowBoundRock1 + rand() % (upBoundRock1 - lowBoundRock1), rightBoundRock1 + rand() % (leftBoundRock1 - rightBoundRock1)));
            }
            for (int i = 0; i < rockCount; i++) {
                pos_rock_2.push_back(glm::vec3(x_coordinate / scale_rock2, lowBoundRock2 + rand() % (upBoundRock2 - lowBoundRock2), rightBoundRock2 + rand() % (leftBoundRock2 - rightBoundRock2)));
            }
            lastTime = myTime;
            numberOfIteration = numberOfIteration - 2.5;
            if (numberOfIteration < 3.5)
            {
                numberOfIteration = 3.5;
            }
        }
    }

    // function to detect a collion with a rock of type 1
    bool detectCollisionsRock1(vector<glm::vec3> positions, glm::vec3 boat_pos) {
        bool detect = false;
        
        for(glm::vec3 rock_pos  : positions)
        {

            
            float bound_y;
            float bound_z;

            // the x coordinate is the same for all the object so this if condition can be deleted 
            if (true)
            {
                if (boat_pos.y * scale_boat >= rock_pos.y * scale_rock1)
                {
                    bound_y = 0.125;
                }
                else
                {
                    bound_y = 0.33;
                }
                if (abs(boat_pos.y * scale_boat - rock_pos.y * scale_rock1) <= bound_y)
                {
                    if (boat_pos.z * scale_boat >= rock_pos.z * scale_rock1)
                    {
                        bound_z = 0.30;
                    }
                    else
                    {
                        bound_z = 0.20;
                    }
                    if (abs(boat_pos.z * scale_boat - rock_pos.z * scale_rock1) <= bound_z)
                    {
                        detect = true;
                    }

                }
                
            }
        }
        return detect;
    }

    // function to detect a collion with a gold boat
    bool detectCollisionsGoldBoat(glm::vec3 pos_gold_boat, glm::vec3 boat_pos)
    {

        float bound_y;
        float bound_z;


        if (boat_pos.y * scale_boat >= pos_gold_boat.y * scale_boat_gold)
        {
            bound_y = 0.035;
        }
        else 
        {
            bound_y = 0.25;
        }
        
        if (abs(boat_pos.y * scale_boat - pos_gold_boat.y * scale_boat_gold) <= bound_y)
        {
            if (boat_pos.z * scale_boat >= pos_gold_boat.z * scale_boat_gold)
            {
                bound_z = 0.145;
            }
            else
            {
                bound_z = 0.130;
            }
            if (abs(boat_pos.z * scale_boat - pos_gold_boat.z * scale_boat_gold) <= bound_z)
            {
                return true;
            }

        }
        return false;
    }

    // function to restart the game setting status game, actual game life, position of the boat and score
    void restartGame()
    {
        actual_state = GAME;
        actual_game_life = 3;
        global_pos_boat = glm::vec3(x_coordinate / scale_boat, 0, 600);
        pos_rock_1.clear();
        pos_rock_2.clear();
        for (int i = 0; i < rockCount; i++) {
            pos_rock_1.push_back(glm::vec3(x_coordinate / scale_rock1, lowBoundRock1 + rand() % (upBoundRock1 - lowBoundRock1), rightBoundRock1 + rand() % (leftBoundRock1 - rightBoundRock1)));
        }
        for (int i = 0; i < rockCount; i++) {
            pos_rock_2.push_back(glm::vec3(x_coordinate / scale_rock2, lowBoundRock2 + rand() % (upBoundRock2 - lowBoundRock2), rightBoundRock2 + rand() % (leftBoundRock2 - rightBoundRock2)));
        }
        
        score = 0.0f;
        
    }

    // function to detect a collion with a rock of type 2
    bool detectCollisionsRock2(vector<glm::vec3> positions, glm::vec3 boat_pos) {
        bool detect = false;

        for (glm::vec3 rock_pos : positions)
        {

            float bound_y;
            float bound_z;

            // the x coordinate is the same for all the object so this if condition can be deleted 
            if (true)
            {
                if (boat_pos.y * scale_boat >= rock_pos.y * scale_rock2)
                {
                    bound_y = 0.20;
                }
                else
                {
                    bound_y = 0.33;
                }
                if (abs(boat_pos.y * scale_boat - rock_pos.y * scale_rock2) <= bound_y)
                {
                    if (boat_pos.z * scale_boat >= rock_pos.z * scale_rock2)
                    {
                        bound_z = 0.39;
                    }
                    else
                    {
                        bound_z = 0.20;
                    }
                    if (abs(boat_pos.z * scale_boat - rock_pos.z * scale_rock2) <= bound_z)
                    {
                        detect = true;
                    }

                }

            }
        }
        return detect;
    }

    // function to write the given score into a file
    bool writeScore(string file_name, string content)
    {
        ofstream of(file_name, ios::out | ios::app | ios::binary);
        if (of.is_open())
        {
            of << content;
            of.close();
            return true;
        }
        
        of.close();

        return false;
    }
    
    // function to read the best score if it is present, 0 otherwise
    float readBestScore(string file_name)
    {
        float bestScore = 0.0f;

        ifstream of(file_name, ios::out | ios::binary);
        if (of.is_open())
        {
            of >> bestScore;
        }
        of.close();
        
        return bestScore;
    }

    // function to clean the file given a file name
    void clearFile(string file_name)
    {
        std::ofstream ofs;
        ofs.open(file_name, std::ofstream::out | std::ofstream::trunc);
        ofs.close();
    }

    // function to check that the boat do not cross the boundaries of our game
    void checkBoatBoundaries() 
    {
        if (global_pos_boat.y > upBound)
        {
            global_pos_boat = glm::vec3(global_pos_boat.x, upBound, global_pos_boat.z);
        }
        if (global_pos_boat.y < lowBound)
        {
            global_pos_boat = glm::vec3(global_pos_boat.x, lowBound, global_pos_boat.z);
        }
        if (global_pos_boat.z > leftBound)
        {
            global_pos_boat = glm::vec3(global_pos_boat.x, global_pos_boat.y, leftBound);
        }
        if (global_pos_boat.z < rightBound)
        {
            global_pos_boat = glm::vec3(global_pos_boat.x, global_pos_boat.y, rightBound);
        }
    }
};


// This is the main: probably you do not need to touch this!
int main() {
    MyProject app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


