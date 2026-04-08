#pragma once



class Camera {
public:
    playerStruct::cameraNode node;



    Camera() {
        node.view = glm::lookAt(glm::vec3(0)/*摄像机坐标*/, glm::vec3(0)/*被观测坐标*/, coordsInfo::worldUp);
        node.rota = { 1.0f, 0.0f, 0.0f, 0.0f };
        node.pos = playerInfo::initSpawnPos;       //设置相机坐标
        node.front = { 0.0f, 0.0f, 0.0f };

        mouseMovement(0, 0);                //不初始化的话，开局不移动没画面
    }

private:
    //float keySpeed = 8.0f;
    float keySpeed = 64.0f;
    //float keySpeed = 128.0f;    //位移速率
    float xMouseSpeed = 0.1f;   //鼠标移动X速率
    float yMouseSpeed = 0.1f;   //鼠标移动Y速率

public:
    //键盘移动处理
    void keyMovement(glm::vec3 direction) {
        static glm::vec3 xAxis{ -1.0f, 0.0f, 0.0f };

        node.pos += keySpeed * (direction.x * glm::vec3{ node.rota * xAxis } - direction.y * node.front + direction.z * glm::vec3{coordsInfo::worldUp});

        updateView();
    }

    //鼠标移动处理
    void mouseMovement(float deltaX, float deltaY) {
        static glm::quat initRota = { 1.0f, 0.0f, 0.0f, 0.0f };
        static glm::vec2 rota_ang = { 0.0f, 0.0f };     //旋转角度
        static glm::vec2 rota_rad = { 0.0f, 0.0f };     //旋转弧度

        static glm::vec3 xAxis{ 1.0f, 0.0f, 0.0f };
        static glm::vec3 yAxis{ 0.0f, 1.0f, 0.0f };
        static glm::vec3 zAxis{ 0.0f, 0.0f,-1.0f };

        rota_ang.x += deltaY * xMouseSpeed;
        rota_ang.y -= deltaX * yMouseSpeed;

        rota_ang.x = glm::clamp(rota_ang.x, -90.0f, 90.0f);
        rota_ang.y = glm::mod(rota_ang.y, 360.0f);
        rota_rad = glm::radians(rota_ang);

        node.rota = glm::angleAxis(rota_rad.y, yAxis) * glm::angleAxis(rota_rad.x, xAxis) * initRota;
        node.front = node.rota * zAxis;

        updateView();
    }

    void updateView() {
        node.view = glm::mat4_cast(glm::conjugate(node.rota)) * glm::translate(glm::mat4(1.0f), glm::vec3(playerInfo::logicOriginPos - node.pos));
    }
};