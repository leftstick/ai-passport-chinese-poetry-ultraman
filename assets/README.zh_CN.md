<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 资源目录（Assets）

本目录集中存放可复用的资源（字库、图片、生成的诗词插画、诗词音频、电池参数等），按资源类型分子目录管理。每个资源放在其类型对应的子目录，并记录放置路径、命名方式、集成方式与来源/许可。二进制资源（字体、图片、音频）不属于纯 markdown 文档，请勿与文档混放。涉及版权/授权的资源需注明来源与许可。

## 字库（fonts）

可复用的字库文件与生成的字库源码放在 `fonts/`。

- 命名要能反映字族、字重、字级与格式。
- 记录来源、许可、字符范围、转换命令与目标放置路径。
- 添加字库前评估 Flash 与内部 RAM 影响；ESP32-C3 无 PSRAM。
- 不提交许可不允许分发的字库。

## 图片（images）

可复用的源图与生成的显示资产放在 `images/`。

| 文件 | 尺寸与格式 | 用途与来源 |
| --- | --- | --- |
| [`images/home.jpg`](images/home.jpg) | 3840 × 2160，JPEG | 嵌入中英文项目 README 的产品主图，突出 AI Passport 产品形象与开放、人人可创作的理念。 |
| [`images/readme-hardware-specs.png`](images/readme-hardware-specs.png) | 2172 × 724，PNG RGBA | 保留为可选技术参考图，不再用于首页主视觉。于 2026-09-17 使用内置图像生成工具为本仓库生成；已根据文档中的硬件能力契约核对图中的六项标签与参数。 |
| [`images/logo-wordmark.png`](images/logo-wordmark.png) | 1648 × 336，PNG RGBA | 从仓库原始 `images/logo.png` 中精确裁切并去除背景的黑色字标；用于中英文项目 README 的浅色主题。 |
| [`images/logo-wordmark-dark.png`](images/logo-wordmark-dark.png) | 1648 × 336，PNG RGBA | 提取字标的白色版本；README 使用 `<picture>` 在 GitHub 深色主题下显示。 |

- 使用描述性命名，并记录尺寸、像素格式、转换步骤与目标路径。
- 优先采用适合 240 × 320 RGB565 显示的格式，并纳入 Flash 与内部 RAM 考量。
- 许可允许时保留可编辑源文件，并记录来源与许可。
- 图片中不得包含设备二维码秘密、凭证或个人数据。

## 诗词插画（poetry art）

冒险场景与 Q 版奥特曼立绘由脚本生成，而不是手工维护的二进制图片。
[`../tools/build_poetry_art.py`](../tools/build_poetry_art.py) 绘制三张 240 × 320
RGB565 背景（春日花溪、月夜窗棂、落日楼台），以及三张 64 × 84 RGB565A8
奥特曼姿态（待机、提示、欢呼），输出 `main/assets/poetry_art.c` 与
`main/assets/poetry_art.h`。

- 修改构图、配色或姿态几何后重新运行生成脚本，不要手改生成的 C 文件。
- 显示契约由 `tools/check_assets.py` 校验：三张 240 × 320 场景、三张 64 × 84
  立绘，并导出访问函数。
- 生成脚本只依赖 Pillow 与 Python 标准库，可离线运行，不给固件增加运行时依赖。

## 诗词音频（audio）

诗词朗读、生成的国风背景音乐与提示音位于 [`audio/`](audio/)，来源与授权说明见
[`audio/SOURCES.zh_CN.md`](audio/SOURCES.zh_CN.md)。

| 路径 | 内容 |
| --- | --- |
| [`audio/narration/poem_7.wav`](audio/narration/poem_7.wav) | 《春晓》普通话朗读（16 kHz、16 位单声道 PCM）。 |
| [`audio/narration/poem_2.wav`](audio/narration/poem_2.wav) | 《静夜思》普通话朗读。 |
| [`audio/narration/poem_16.wav`](audio/narration/poem_16.wav) | 《登鹳雀楼》普通话朗读。 |

[`../tools/build_poetry_audio.py`](../tools/build_poetry_audio.py) 对已提交的朗读录音做
归一化，合成 12.8 秒五声音阶背景循环与五个短提示音（移动、确认、答对、答错、
解锁），并写入 `main/audio/poetry_audio.c` 与 `main/audio/poetry_audio.h`。

- 保持可校验的容量上限：`tools/check_assets.py` 将嵌入的诗词音频限制在
  1,200,000 个采样点以内，确保 factory 分区仍能容纳固件。
- 朗读使用 Windows SAPI（`Microsoft Huihui Desktop` 中国大陆语音，语速 −2）重新生成；
  已提交的 WAV 是重建依据，其他平台无需 SAPI 即可构建固件。
- 无再分发许可不提交媒体文件。

## 电池参数（battery）

[`battery/wanli-500mah-profile2-xil40x-20261020.txt`](battery/wanli-500mah-profile2-xil40x-20261020.txt)
保存万锂新能源 500 mAh 电芯的 CW2017 参数（profile2、XIL40X、2026-10-20）。
该文件是项目提供的 `喜梨_智能工卡_万锂新能源500mAh_profile2_XIL40X_20261020.txt`
的原样副本，按维护者要求保留用于本次固件更新；原文件没有附带单独的许可说明。

格式为 80 行两位十六进制数字，每行一个字节，按寄存器顺序排列。
BSP 在 `components/bsp/src/bsp_battery.c` 中嵌入这 80 字节，主机检查会验证
源文件及数组内容完全一致。文本文件 SHA-256：
`7ea96eba4634f4495a37c25ffacd54c1deaa0693b162c892d671a2ba68460919`。
解码后 80 字节 SHA-256：
`fa7365671093ef449c5bdcfb4adb279e3d275f888109f50b6b2f21e9b96220d5`。
这是电量计参数，不是充电配置，也不能通用于所有 500 mAh 电芯；详见
[硬件指南](../docs/hardware-design/AI_HARDWARE_DEVELOPMENT_GUIDE.zh_CN.md#9-cw2017-电池计)。
