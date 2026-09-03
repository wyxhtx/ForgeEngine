# ForgeEngine
## 简介
	铸造引擎，C++20 + Vulkan

## 构建
	cmake --preset vs
	cmake --build --preset release
	.\build\vs\bin\output\x64\Release\ForgeEngine.exe

## 指令
	项目路径下自动加载自定义指令，完整指令表见 docs/调试指令速查.md
	
	常用指令：
	`fe-br`		构建+运行 
	`fe-config`	配置速览 
	`fe-st`		git 状态

## 目录
	cmd		指令	
	config		配置	
	docs		文档
	src/assets	资源	
	src/code	源码	
	build		构建