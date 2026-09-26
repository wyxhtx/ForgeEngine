#pragma once



//engine    枚举空间
namespace engineEnum {
	namespace tickEnum {
		enum type {
			key,        //按键获取刷新率
			uiText,     //文本UI刷新率
			plate,      //板块更新刷新率
			cube,       //方块更新刷新率

			count
		};
	}
}



//ui        枚举空间
namespace uiEnum {
	//uiText锚点枚举
	namespace uiTextAnchorEnum {
		enum type {
			topLeft,     //左上：行标签列左对齐，值区跨行列对齐
			topRight,    //右上：整行右端对齐视口右缘

			count
		};
	}
}



//render    枚举空间
namespace renderEnum {
	//render附件枚举
	namespace renderAttId {
		enum type {
			forward_ra,     //前向渲染附件
			deferred_ra,    //延迟渲染附件
			shadowMap_ra,   //阴影映射附件

			count
		};
	}

	//render帧图像枚举
	namespace renderFrameId {
		enum type {
			conv_rf,        //常规：颜色、深度、视图
			swap_rf,        //交换链

			count
		};
	}

	//render模板枚举
	namespace renderTemplateId {
		enum type {
			swapChain_rt,   //交换链
			offScreen_rt,   //离屏
			deferred_rt,    //延迟渲染
			shadowMap_rt,   //阴影映射

			count
		};
	}

	//render实例枚举
	namespace renderInstanceId {
		enum type {
			scScence_ri,    //交换链场景
			osScence_ri,    //离屏场景
			osMirror_ri,    //离屏镜面
			dsScence_ri,    //延迟渲染场景
			shadowMap_ri,   //阴影映射

			count
		};
	}
}

//pipeline		枚举空间
namespace pipelineEnum {
	//primitive	枚举（图元）
	namespace primitiveId {
		enum type {
			// ---- 基础图元 ----
			pointList,					//点列表										每个顶点画成一个点：					粒子、调试点云
			lineList,					//线段列表										每 2 顶点一条独立线段：					线框、选择框描边
			lineStrip,					//线段条带										顶点串成折线：							轨迹、曲线可视化
			triangleList,				//三角形列表									每 3 顶点一个三角形：					网格/地形/模型主力
			triangleStrip,				//三角形条带									连续三角形：							UI 四边形、带状网格
			triangleFan,				//三角形扇										首顶点共享：							扇形/锥体网格（GL 遗留，条带 + 退化索引可替代）

			// ---- 邻接图元（需 geometryShader） ----
			lineListWithAdjacency,		//邻接线段列表									每 4 顶点一条线段并带两侧邻接点：		几何着色器外扩用
			lineStripWithAdjacency,		//邻接线段条带									同上，串成折线
			triangleListWithAdjacency,	//邻接三角形列表								每 6 顶点一个三角形 + 3 邻接点：		轮廓/法线外扩
			triangleStripWithAdjacency,	//邻接三角形条带								同上，条带形式

			// ---- 细分图元（需 tessellationShader） ----
			patchList,					//面片列表										每 N 顶点一个面片：						细分着色器输入

			count
		};
	}

	//raster	枚举（光栅）
	namespace rasterId {
		enum type {
			// ---- 填充模式 ----
			fill_none_ccw,				//填充·不剔·逆时针								双面薄片（树叶/旗帜）：					逆时针为正面
			fill_none_cw,				//填充·不剔·顺时针								同上：									顺时针为正面
			fill_front_ccw,				//填充·剔正面·逆时针							内部观察（天空盒内侧/内壁）、镜像渲染
			fill_front_cw,				//填充·剔正面·顺时针							同上：									顺时针为正面
			fill_back_ccw,				//填充·剔背面·逆时针							不透明实体标准配置：					省一半片元
			fill_back_cw,				//填充·剔背面·顺时针							同上：									顺时针为正面（镜像资源）
			fill_front_and_back,		//填充·双面·逆时针								双面都保留（绕序无效）：				厚薄片、透明排序

			// ---- 线框模式（需 fillModeNonSolid、dynamicId::viewportScissorLineWidth） ----
			line_none_ccw,				//线框·不剔·逆时针								全边线框、可看穿：						调试/技术图；
			line_none_cw,				//线框·不剔·顺时针								同上：									顺时针为正面；
			line_front_ccw,				//线框·剔正面·逆时针							只画背向边：							内轮廓（少用）；
			line_front_cw,				//线框·剔正面·顺时针							同上（少用）；
			line_back_ccw,				//线框·剔背面·逆时针							只画正向边：							选择框描边、隐藏线；
			line_back_cw,				//线框·剔背面·顺时针							同上：									顺时针为正面；
			line_front_and_back,		//线框·双面·逆时针								双面都画线：							全部边线（不剔）；

			// ---- 点模式（需 fillModeNonSolid；cull 与绕序仍生效） ----
			point_none_ccw,				//点·不剔·逆时针								顶点可视化：							把已有网格的顶点画成点
			point_none_cw,				//点·不剔·顺时针								同上：									顺时针为正面
			point_front_ccw,			//点·剔正面·逆时针								只画背向顶点：							内面顶点可视化
			point_front_cw,				//点·剔正面·顺时针								同上：									顺时针为正面
			point_back_ccw,				//点·剔背面·逆时针								只画正向顶点：							外表面顶点可视化
			point_back_cw,				//点·剔背面·顺时针								同上：									顺时针为正面
			point_front_and_back,		//点·双面·逆时针								双面都画点：							全部顶点可视化

			// ---- 特殊 ----
			fill_back_ccw_clampDepth,	//填充·剔背面·逆时针·夹紧深度					深度夹紧进 [0,1]：						深度精度不足/大幅近平面时防穿透（需 depthClamp）
			discardAll,					//丢弃全部图元									rasterizerDiscardEnable=true：			只走顶点/网格阶段（只写时间戳、纯测性能）

			count
		};
	}

	//bias		枚举（深度偏移，raster -> depthBias）
	namespace biasId {
		enum type {
			none,						//不偏移										不启用深度偏移：						主通道默认
			polygonOffset,				//多边形偏移									常量 1.0 / 斜率 1.0 / 不夹紧（选择框覆盖、共面贴合）
			shadowOffset,				//阴影斜率偏移									常量 0.0 / 斜率 1.0 / 不夹紧（阴影贴图防 acne 常用）

			count
		};
	}

	//depth		枚举（深度）
	namespace depthId {
		enum type {
			// ---- 测 + 写 ----
			test_never,					//测·永不通过									仅模板流程：							强制走 depthFail 分支
			test_write_less,			//测·写·小于									不透明物体标准（近者胜 + 提供可读深度）
			test_write_equal,			//测·写·等于									共面/等深二次绘制
			test_write_lessEqual,		//测·写·≤										共面贴合、阴影深度比较
			test_write_greater,			//测·写·大于									reversed-Z（反向深度）
			test_write_notEqual,		//测·写·不等									遮罩式绘制
			test_write_greaterEqual,	//测·写·≥										reversed-Z 共面
			test_write_always,			//测·写·始终通过								深度预通道

			// ---- 测 + 不写 ----
			test_noWrite_less,			//测·不写·小于									只测不写（遮挡剔除）
			test_noWrite_equal,			//测·不写·等于									等深处叠加、不改深度
			test_noWrite_lessEqual,		//测·不写·≤										天空盒/背景层
			test_noWrite_greater,		//测·不写·大于									reversed-Z 只测不写
			test_noWrite_notEqual,		//测·不写·不等									深度不一致处显示（遮罩）
			test_noWrite_greaterEqual,	//测·不写·≥										reversed-Z 只测不写（共面）
			test_noWrite_always,		//测·不写·始终通过								永远可见前景层；与 noDepth 等价

			// ---- 不参与深度 ----
			noDepth,					//不参与深度									纯 2D/UI 与后处理（compare 被忽略占位）

			// ---- 深度范围测试（需 depthBounds feature；边界值走动态状态） ----
			test_write_less_bounds,		//测·写·小于·范围								级联阴影：								只处理本层深度切片
			test_noWrite_less_bounds,	//测·不写·小于·范围								同上但不写深度

			count
		};
	}

	//stencil	枚举（模板）
	namespace stencilId {
		enum type {
			// ---- 默认 ----
			none,						//不测试										stencilTestEnable=false，其余字段被忽略

			// ---- 遮罩类 ----
			writeRef1,					//标记通道										写参考值 1：							描边/遮罩的第一步
			drawEqual1,					//仅标记处绘制									只画被标记区域（遮罩内绘制）
			drawNotEqual1,				//仅未标记处绘制								描边外扩、遮罩外绘制
			drawNotEqual0,				//仅已写入处绘制								通用遮罩外绘制

			// ---- 阴影体 ----
			zPass,						//阴影体										Z-pass 正面 passOp=INCR_WRAP、	   背面 passOp=DECR_WRAP，	   compare=ALWAYS
			zFail,						//阴影体										Z-fail 正面 depthFailOp=INCR_WRAP、背面 depthFailOp=DECR_WRAP，compare=ALWAYS

			count
		};
	}

	//blend		枚举（颜色混合）
	namespace blendId {
		enum type {
			// ---- 基础合成 ----
			opaque,						//不透明										直接覆盖：								enable=false，因子被忽略（占位）
			alphaBlend,					//常规半透明									玻璃/水面/粒子/UI：						src*a + dst*(1-a)
			premultipliedAlpha,			//预乘											Alpha 预乘纹理正确合成：				src   + dst*(1-a)，无黑边

			// ---- 加 / 减 ----
			additive,					//alpha											调制叠加 光晕/火焰/能量：				src*a + dst
			additiveNoAlpha,			//纯叠加										辉光/激光/亮斑：						src   + dst
			subtract,					//相减											dst - src：								差值、去色类效果
			subtractAlpha,				//按											alpha 相减 dst - src*a：				遮罩相减

			// ---- 乘 / 滤色 ----
			multiply,					//正片叠底										src*dst：								阴影投射、染色、暗角
			multiplyAlpha,				//带											alpha 正片叠底 src*dst + dst*(1-a)：	透明贴图乘底
			screen,						//滤色											src + dst - src*dst：					泛光、亮部叠加

			// ---- 取暗 / 取亮 ----
			darken,						//变暗											逐通道取小：							NPR、光照贴图取暗部
			lighten,					//变亮											逐通道取大：							NPR、光照贴图取亮部

			count
		};
	}

	//writeMask	枚举（颜色写入，blend -> colorWriteMask）
	namespace writeMaskId {
		enum type {
			rgba,						//写											RGBA 默认：颜色与透明度都写
			rgb,						//只写											RGB 保留目标 alpha（后续合成/透明度累积）
			alphaOnly,					//只写											alpha 只生成透明度通道（透明通道预通道）
			none,						//不写颜色										颜色附件仅供深度/模板流程用（占位）

			count
		};
	}

	//logicOp	枚举（颜色运算，blend -> logicOp）
	namespace logicOpId {
		enum type {
			disable,					//不启用										走混合公式					（默认）
			copy,						//直接覆盖										源颜色直接写入，不做混合运算（需 logicOp）
			invert,						//颜色取反										写入取反后的目标颜色		（位运算）
			xorOp,						//异或											源与目标逐位异或			（位运算，特异效果）

			count
		};
	}

	//dynamic	枚举（动态状态）
	namespace dynamicId {
		enum type {
			// ---- 基础 ----
			none,						//无动态状态

			// ---- 视口 + 裁剪 ----
			viewportScissor,			//视口 + 裁剪 交换链重建、多分辨率、分屏渲染无需重建管线

			// ---- 按需叠加 ----
			viewportScissorLineWidth,	// + 线宽 仅 line_* 光栅预设有效：				线框按需调线宽
			viewportScissorDepthBias,	// + 深度偏移 仅 depthBiasEnable=TRUE 有效：	阴影/贴花按需调偏移
			viewportScissorBounds,		// + 深度范围 配合深度范围预设：				级联阴影逐级设边界
			viewportScissorRasterDepth,	// + 光栅/深度批处理							一条管线服务多材质/双面/多图元（1.3 核心）
			viewportScissorAll,			// + 全部可选									上述并集，再加常量混合与模板动态状态

			count
		};
	}



	//shader	枚举
	namespace shaderId {
		//顶点	类型
		namespace vert {
			enum type {
				vert,					//顶点着色器											
				frag,					//片段着色器											

				count
			};
		}

		//网格	类型
		namespace mesh {
			enum type {
				task,					//任务着色器											
				mesh,					//网格着色器											
				frag,					//片段着色器											

				count
			};
		}
	}

	//cmdDraw	枚举
	namespace cmdDrawId {
		enum type {
			indexed,					//索引绘制												
			fullscreen,					//全屏三角形											
			text,						//UI文本												

			count
		};
	}



	//pipeline	枚举
	namespace procPipelineId {
		enum type {
			terrain_p,					//地形													
			model_p,					//模型													
			skybox_p,					//天空盒												
			mine_p,						//选择框												
			uiText_p,					//文本ui												
			osMirror_p,					//离屏镜面												
			osScence_p,					//离屏场景												
			dsScence_p,					//延迟渲染场景											
			shadowMap_p,				//阴影映射												

			count
		};
	}
}



//todo 后续慢慢拆分
//buffer    枚举空间
namespace bufferEnum {
	namespace textAlignId {
		enum type {
			left,
			center,
			right
		};
	}

	namespace procDescriptorId {
		enum type {
			scenceForward_d,    //场景前向描述符
			scenceDeferred_d,   //场景延迟描述符，用于将纹理贴图和法线贴图转换为3大附件
			uiText_d,           //文本描述符
			dsScence_d,         //延迟渲染描述符，用于将3大附件和光照处理成最终结果
			shadowMap_d,        //阴影映射描述符

			count
		};
	}

	namespace procSamplerId {
		enum type {
			terrain_s,          //地形采样器
			staticModel_sn,     //静态模型邻近采样器
			staticModel_sl,     //静态模型线性采样器
			dynamicModel_sn,    //动态模型邻近采样器
			dynamicModel_sl,    //动态模型线性采样器
			uiText_s,           //文本采样器
			offScreen_s,        //离屏渲染采样器
			shadowMap_s,        //阴影映射采样器

			count
		};
	}

	namespace procStaticModelId {
		enum type {
			temple_m,   //神庙

			count
		};
	}

	namespace procDynamicModelId {
		enum type {
			droid_m,		//机器人

			count
		};
	}
}