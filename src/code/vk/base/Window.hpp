#pragma once



class Window {
public:
	Camera InitCamera{};

	GLFWwindow* window;
	bool isMin = true;



	Window() {
		glfwInit();														//初始化 GLFW 库
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);					//设置窗口的客户端 API 为无图形 API
		glfwWindowHint(GLFW_RESIZABLE, windowInfo::resizableEnabled);	//设置窗口是否可调整大小，根据 windowInfo::resizableEnabled 值来决定

		monitor = glfwGetPrimaryMonitor();								//获取主显示器（即系统的主显示器）
		mode = glfwGetVideoMode(monitor);								//获取主显示器的当前显示模式（分辨率等信息）

		//计算窗口在屏幕上的位置（居中）
		windowX = (mode->width - windowInfo::width) / 2;
		windowY = (mode->height - windowInfo::height) / 2;

		window = glfwCreateWindow(width, height, windowInfo::engineName, nullptr, nullptr);	//创建窗口，传入指定的宽度、高度和名称，且不与任何 API 上下文关联
		glfwSetWindowPos(window, windowX, windowY);											//设置窗口位置到屏幕中心
		glfwSetCursorPos(window, windowInfo::width / 2.0, windowInfo::height / 2.0);		//设置鼠标光标的位置到窗口中心

		//禁用鼠标悬浮模式，强制将鼠标隐藏在窗口内并锁定
		glfwSetWindowUserPointer(window, this);
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		GetKeyboardLayoutName(keyboardLayout);												//获取原始键盘布局
		LoadKeyboardLayout(L"00000409", KLF_ACTIVATE);										//设置美式键盘布局

		glfwSetKeyCallback(window, KeyCallbackStatic);										//键盘回调函数
		glfwSetCursorPosCallback(window, mouse_callback);									//鼠标回调函数
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);					//窗口回调函数

		if (debugInfo::LogSwitch) {
			std::cout << "\t当前键盘布局" << ":";
			std::wcout << L"" << keyboardLayout << std::endl;

			Log::printTitle("————————Window已创建");

			Log::delimiter();
		}
	}

	~Window(){
		LoadKeyboardLayout(keyboardLayout, KLF_ACTIVATE);								//恢复原始键盘布局

		glfwDestroyWindow(window);
		glfwTerminate();

		if (debugInfo::LogSwitch) {
			Log::print("Window已清除");
		}
	}

private:
	GLFWmonitor* monitor;
	const GLFWvidmode* mode;

	//窗口初始大小
	int width = windowInfo::width;
	int height = windowInfo::height;

	//窗口实际大小
	int windowX;
	int windowY;

	//原始键盘布局
	WCHAR keyboardLayout[KL_NAMELENGTH];



	//按键回调
	static void KeyCallbackStatic(GLFWwindow* window, int key, int scancode, int action, int mods) {
		// 从Window指针中获取实例指针
		Window* instance = static_cast<Window*>(glfwGetWindowUserPointer(window));
		if (instance) {
			// 调用实例的成员函数处理键盘事件
			instance->KeyCallBack(window, key, scancode, action, mods);
		}
	}

	//鼠标回调
	static void mouse_callback(GLFWwindow* window, double xPos, double yPos) {
		Window* instance = static_cast<Window*>(glfwGetWindowUserPointer(window));
		if (instance) {
			instance->MouseCallBack(window, (float)xPos, (float)yPos);
		}
	}

	//窗口大小变化回调
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		Window* instance = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));

		if (width == 0 || height == 0) {
			instance->isMin = false;
		}
		else {
			instance->isMin = true;
			instance->width = width;
			instance->height = height;
		}
	}



	//处理按键回调，只适用于不需要长按的按钮
	void KeyCallBack(GLFWwindow* window, int key, int scancode, int action, int mods) {
		if (action == GLFW_PRESS) {
			switch (key) {
			case GLFW_KEY_F11:									//全屏
				while (lockInfo::updatePlateLock.load());		//等待地形线程结束
				glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
				break;
			case GLFW_KEY_ESCAPE:								//退出全屏
				while (lockInfo::updatePlateLock.load());		//等待地形线程结束
				glfwSetWindowMonitor(window, nullptr, windowX, windowY, windowInfo::width, windowInfo::height, mode->refreshRate);
				break;
			case GLFW_KEY_END:									//结束游戏，使用笔记本时得和fn组合使用
				while (lockInfo::updatePlateLock.load());		//等待地形线程结束
				glfwSetWindowShouldClose(window, GLFW_TRUE);
				break;
			}
		}
	}

	//处理鼠标位移回调
	void MouseCallBack(GLFWwindow* window, float xPos, float yPos) {
		//鼠标移动位置
		static float lastX = windowInfo::width / 2;
		static float lastY = windowInfo::height / 2;

		//计算位移
		static float deltaX;
		static float deltaY;

		deltaX = xPos - lastX;
		deltaY = lastY - yPos;

		lastX = xPos;
		lastY = yPos;

		if (deltaX != 0 || deltaY != 0) {
			InitCamera.mouseMovement(deltaX, deltaY);
		}
	}

public:
	//获取键盘按键，适用于需要长按的按钮
	void getKey(float deltaTime) {
		glm::vec3 velocity{ 0.f };

		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			velocity.x =  1.0f;
		}
		else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			velocity.x = -1.0f;
		}

		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			velocity.y =  1.0f;
		}
		else if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			velocity.y = -1.0f;
		}

		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
			velocity.z =  1.0f;
		}
		else if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
			velocity.z = -1.0f;
		}

		if (glm::length(velocity) != 0.f) {
			//参考公式：deltaX = V * deltaTime
			InitCamera.keyMovement(velocity * deltaTime);
		}

		//左键检测
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
			playerInfo::mouseButtonLeft = true;
		}
		else{
			playerInfo::mouseButtonLeft = false;
		}
	}
};