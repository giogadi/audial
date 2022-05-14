#include <iostream>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_access.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "matrix.h"

int main() {
    // glm::mat4 glmPersp = glm::perspective(
    //     glm::radians(45.f), 800.f / 600.f, 0.001f, 100.f);
    
    // Mat4 myPersp = GetPerspectiveMat(45.f * 3.14159265359f / 180.f, 800.f / 600.f, 0.001f, 100.f);

    // std::cout << "GLM:" << std::endl;
    // for (int i = 0; i < 4; ++i) {
    //     for (int j = 0; j < 4; ++j) {
    //         std::cout << glmPersp[i][j] << " ";
    //     }
    //     std::cout << std::endl;
    // }

    // std::cout << std::endl;
    // std::cout << "Mine:" << std::endl;
    // for (int i = 0; i < 4; ++i) {
    //     for (int j = 0; j < 4; ++j) {
    //         std::cout << myPersp._data[4*i+j] << " ";
    //     }
    //     std::cout << std::endl;
    // }

    // glm::mat4 glmLookAt = glm::lookAt(glm::vec3(1.f, 2.f, 3.f), glm::vec3(3.f, 2.f, 1.f), glm::vec3(0.f, 1.f, 0.f));
    // Mat4 myLookAt = LookAt(Vec3(1.f,2.f,3.f), Vec3(3.f,2.f,1.f), Vec3(0.f,1.f,0.f));

    // // glm::mat4 glmLookAt = glm::lookAt(glm::vec3(0.f, 0.f, 3.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
    // // Mat4 myLookAt = LookAt(Vec3(0.f,0.f,3.f), Vec3(0.f,0.f,0.f), Vec3(0.f,1.f,0.f));

    // std::cout << "GLM:" << std::endl;
    // for (int i = 0; i < 4; ++i) {
    //     for (int j = 0; j < 4; ++j) {
    //         std::cout << glmLookAt[i][j] << " ";
    //     }
    //     std::cout << std::endl;
    // }

    // std::cout << std::endl;
    // std::cout << "Mine:" << std::endl;
    // for (int i = 0; i < 4; ++i) {
    //     for (int j = 0; j < 4; ++j) {
    //         std::cout << myLookAt._data[4*i+j] << " ";
    //     }
    //     std::cout << std::endl;
    // }

    glm::mat4 aGlm, bGlm;
    Mat4 a, b;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            a._data[4*i + j] = aGlm[i][j] = 4*i + j;
            b._data[4*i + j] = bGlm[i][j] = 4*i + j;
        }
    }
    glm::mat4 cGlm = aGlm * bGlm;
    Mat4 c = a * b;

    // glm::mat4 aGlm(1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2);
    // glm::mat4 bGlm(2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1);
    // glm::mat4 cGlm = aGlm * bGlm;

    // Mat4 a(1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2);
    // Mat4 b(2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1);
    // Mat4 c = a * b;

    std::cout << "GLM:" << std::endl;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            std::cout << cGlm[i][j] << " ";
        }
        std::cout << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Mine:" << std::endl;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            std::cout << c._data[4*i+j] << " ";
        }
        std::cout << std::endl;
    }


    return 0;
}