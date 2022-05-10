#include <iostream>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_access.hpp"

int main() {
    glm::mat4 m(1.f);
    glm::vec3 v(glm::column(m,2));
    std::cout << v[0] << " " << v[1] << " " << v[2] << std::endl;
}