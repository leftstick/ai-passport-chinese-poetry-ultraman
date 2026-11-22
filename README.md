# 奥特曼诗词收集冒险

面向 FoloToy AI-Passport 的慢节奏诗词收集冒险。遗忘怪兽把古诗打散在山水之间，光之勇士嘉乐与奥特曼一起走进诗里的世界，把散落的字句重新拼成完整的诗篇，收入「光之诗库」。

这不是数学速答游戏的换皮：每个关卡只围绕一首诗展开，目标是把这首诗完整地收集下来，而不是刷分数。答题失误不会扣命、不会结束关卡，第二次答错会自动出现诗句提示，保护孩子的兴趣。

本应用基于 [FoloToy/ai-passport](https://gitee.com/FoloToy/ai-passport) 的 BSP 与工程框架开发。按键分发、屏幕渲染与像素绘制思路参考 [leftstick/ai-passport-match-ultraman](https://github.com/leftstick/ai-passport-match-ultraman)，但玩法状态机、关卡流程、界面、题库、音频与存档全部重做，没有沿用其卡牌匹配逻辑，也没有复用上游 demo 的菜单与视觉外壳。逐项差异见 [玩法差异说明](docs/gameplay-difference.zh_CN.md)。

## 硬件与操作

- 主控：ESP32-C3，8 MB Flash，无 PSRAM，ESP-IDF 5.5.3
- 屏幕：240 × 320 竖屏，SPI ST7789P3
- 按键：只有 UP / DOWN / OK 三颗实体按键，无触屏

| 页面 | 操作 |
| --- | --- |
| 首页 | UP / DOWN 选择「继续冒险 / 光之诗库 / 速答挑战 / 音效」，OK 进入或切换静音 |
| 诗篇地图 | UP / DOWN 选择诗篇，OK 进入，长按 OK 返回首页 |
| 诗篇序章 | OK 开始收集，长按 OK 返回地图 |
| 收集挑战 | UP / DOWN 移动光标，OK 选择；长按 OK 返回地图 |
| 字块排序 | OK 选中一个字块，再按 OK 与光标处交换；光标移到「检查」后 OK 提交 |
| 收集奖励 | OK 继续，长按 OK 返回地图 |
| 光之诗库 | UP / DOWN 翻看，OK 查看已收集诗篇，长按 OK 返回首页 |
| 诗篇详情 | OK 重听朗读，长按 OK 返回图鉴 |
| 速答挑战 | UP / DOWN 选择，OK 作答，长按 OK 返回首页；结算页 OK 回首页 |

## 玩法

- **三首诗垂直切片**：《春晓》《静夜思》《登鹳雀楼》。每首诗是一段独立的小旅程，有专属国风场景和奥特曼开场白。
- **四种收集挑战**，每首诗固定按以下顺序出现：
  1. **字块排序**：把打乱的字块交换回正确诗句，再移到「检查」提交。
  2. **下一句接龙**：从三个选项中接出下一句。
  3. **看画猜诗**：根据国风插画从三个诗名中选出对应的一首。
  4. **补全诗句**：为诗句中的空缺字选择正确汉字。
- **无失败惩罚**：答错只播放轻柔提示音，可以继续尝试；同一题第二次答错会自动显示诗句线索。
- **收集奖励**：完成四道挑战后展示完整诗篇、白话解释和奥特曼的收藏台词，同时播放整首朗读，并把诗篇写入图鉴。
- **光之诗库**：3 首诗的收集进度持久化到 NVS；已收集的诗可以随时打开重读、重听。
- **奥特曼微故事**：收集第 1、2、3 首诗时分别解锁一段短故事，讲嘉乐和奥特曼如何理解诗里的风景。
- **速答挑战**：可选模式，15 秒一题的轻量复习，答完自动进入下一题，最后给出本轮得分；它只是复习工具，不替代主线收集。

## 光之诗库

- 诗词源文件为 `assets/poems/poem_library.json`，构建时由 `tools/build_poem_library.py` 生成 `main/assets/poem_library.c`。
- 当前收录 82 首：优先库 55 首（小学 1~3 年级课内必背，以二年级为主），拓展库 27 首（小学 4~6 年级）。
- 本次冒险选取其中 3 首作为垂直切片；题库结构仍按 1000 首目标设计，继续追加 JSON 批次后重新生成即可。
- 题型生成位于 `main/core/poem_challenge.c`，排序、接龙、看画猜诗、补全四种挑战都由纯 C 逻辑生成，可在主机上测试。
- 中文字体使用 Noto Sans SC / Source Han Sans SC 子集，共 4 款字号（12 / 14 / 20 / 28），覆盖 1084 个非 ASCII 字符。UI 文案由 `tools/collect_glyphs.py` 汇总，`tools/check_font_coverage.py` 在构建前校验字形覆盖。

## 画面与音频

- **画面**：三张 240 × 320 国风场景（春日花溪、月夜窗棂、落日楼台）与三张 64 × 84 Q 版奥特曼立绘（待机、提示、欢呼），由 `tools/build_poetry_art.py` 用 Pillow 离线绘制并生成 RGB565/RGB565A8 数据，不依赖网络或外部图库。
- **朗读**：三首诗各有普通话整诗朗读，使用 Windows 中文语音合成后归一化为 16 kHz、16 位单声道 PCM。
- **音乐与提示音**：12.8 秒五声音阶国风循环背景音乐，以及移动、确认、答对、答错、解锁五个轻柔提示音，全部由 `tools/build_poetry_audio.py` 合成。
- **混音**：朗读、提示音和背景音乐三路混音；朗读时背景音乐自动降低音量，提示音不会切断朗读。
- **静音**：首页可随时开关音效，状态写入 NVS，下次开机保持。

素材来源与许可见 [assets/audio/SOURCES.md](assets/audio/SOURCES.md)，资源目录说明见 [assets/README.md](assets/README.md)。

## 构建

需要已激活的 ESP-IDF 5.5.3 环境：

```bash
# 只跑仓库检查与主机测试
./tools/validate.sh --static

# 只构建固件并校验合并镜像
./tools/validate.sh --firmware

# 完整门禁
./tools/validate.sh
```

构建产物：

- 应用固件：`build/FoloToy-AI-Passport.bin`
- 合并固件：`build/FoloToy-AI-Passport-full.bin`，从 `0x0` 整片刷写：

```bash
esptool.py -p <PORT> write_flash 0x0 build/FoloToy-AI-Passport-full.bin
```

`build/`、本地工作记忆 `.ai-memory/` 与文档快照 `.writing/` 不纳入版本控制。

## 下载

已发布固件见 [GitHub Releases](https://github.com/leftstick/ai-passport-chinese-poetry-ultraman/releases)。下载 `FoloToy-AI-Passport-full.bin`，按上一节的命令从 `0x0` 整片刷写。

## 真机试玩

按 [真机测试用例](docs/device-test-plan.zh_CN.md) 逐项检查首页、四种挑战、图鉴、微故事、速答模式、静音与断电记忆。测试时建议使用 3.7 V 电池或稳定的 USB 供电，并确认设备已从 `0x0` 刷入完整合并镜像。

## 目录结构

```text
main/
  app/        页面、布局、按键分发与冒险流程
  core/       冒险状态机、挑战生成、诗篇文本工具、图鉴存档（纯 C）
  audio/      三路混音播放器与生成的诗词音频数据
  assets/     诗词库、国风场景与奥特曼立绘生成数据
  fonts/      中文字体子集
assets/
  poems/      诗词 JSON 源文件
  audio/      朗读 WAV、音频来源与许可说明
  fonts/      字形清单与字体许可
docs/         开发文档、玩法差异与真机测试用例
tests/        可在主机运行的核心逻辑测试
tools/        素材生成、校验与固件归档脚本
```

## 验证状态

最近一次本地完整验证（2026-11-22）：

```text
Build: PASS (3,688,480 bytes app, 56% factory partition free)
Host tests: PASS (w64devkit GCC 16.2.0)
Device tests: NOT RUN
Unverified: 真机显示、按键手感、扬声器音量、长时间稳定性
```

已通过：仓库文本检查、诗词库生成校验、字形清单、字体覆盖、资源表校验、全部主机测试、ESP-IDF 5.5.3 固件构建、合并镜像校验与固件归档。尚未刷写真机，因此显示、按键与音频的实际效果仍需按测试用例确认。

## 许可与致谢

- 工程框架与 BSP：[FoloToy/ai-passport](https://gitee.com/FoloToy/ai-passport)
- 按键与渲染参考：[leftstick/ai-passport-match-ultraman](https://github.com/leftstick/ai-passport-match-ultraman)
- 字体：Noto Sans SC / Source Han Sans SC，许可见 `assets/fonts/SourceHanSansSC-LICENSE.txt`
- 朗读：Windows 中文语音合成，录音文件随仓库提交；音乐与提示音由本项目脚本合成
- 音频来源与许可：[assets/audio/SOURCES.md](assets/audio/SOURCES.md)
