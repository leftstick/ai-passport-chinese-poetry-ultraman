# 奥特诗词闯关

面向 FoloToy AI-Passport 的诗词答题冒险游戏。遗忘怪兽打散了古诗碎片，光之勇士嘉乐与奥特曼并肩作战，用一句句古诗修复光之诗库。

本应用基于 [FoloToy/ai-passport](https://gitee.com/FoloToy/ai-passport) 的 BSP 与工程框架开发，玩法与界面参考 [leftstick/ai-passport-match-ultraman](https://github.com/leftstick/ai-passport-match-ultraman) 的渲染、像素精灵与按键框架后完全重做，没有沿用其卡牌匹配逻辑，也没有复用上游 demo 的菜单与视觉外壳。

## 硬件与操作

- 主控：ESP32-C3，8 MB Flash，无 PSRAM，ESP-IDF 5.5.3
- 屏幕：240 × 320 竖屏，SPI ST7789P3
- 按键：只有 UP / DOWN / OK 三颗实体按键，无触屏

| 操作 | 作用 |
| --- | --- |
| UP / DOWN | 切换选项、移动光标、翻页 |
| OK | 确认答案、进入页面、开始关卡 |
| 长按 OK | 暂停 / 返回 |
| 长按 DOWN | 使用提示 |
| 长按 UP | 必杀槽满时释放必杀 |

答题窗口最后 150 ms 内按下方向键会缓存该操作，在下一题生效，避免连续作答时丢输入。

## 玩法

- **主线闯关**：9 个星球关卡，难度按年级带宽递进，最终关为「光之诗核·遗忘终章」。每关有独立怪兽、题量、答题时间与特殊规则（无提示、半暴击、护盾 QTE、破甲、狂暴等）。
- **战斗流程**：答对推进攻击并积累必杀槽；暴击需要在标记进入判定区时按 OK；必杀触发节奏按键；Boss 战包含光束对拼。
- **难度**：星光 / 烈焰 / 究极三档，分别调整答题时间、生命、怪兽血量和年级档位。
- **限时猎手**：60 秒限时模式，题目会定期换位，按得分与连击结算。
- **收藏图鉴**：查看诗词、英雄、怪兽收录进度与错句回顾。
- **硬币商店**：用关卡金币兑换英雄与道具。
- **周目系统**：通关后进入下一周目，怪兽池轮换并给出周目评价。
- **设置**：音量、震动等选项，全部使用三键操作。

## 光之诗库

- 诗词源文件为 `assets/poems/poem_library.json`，构建时由 `tools/build_poem_library.py` 生成 `main/assets/poem_library.c`。
- 当前收录 82 首：优先库 55 首（小学 1~3 年级课内必背，以二年级为主），拓展库 27 首（小学 4~6 年级）。
- 数据结构按 1000 首目标设计，支持继续追加 JSON 批次后重新生成。
- 题型：诗句接龙（上句选下句 / 下句选上句）、篇名专项、作者专项，以及按关卡切换的混合模式。
- 选项与诗句长度都按 240 像素宽度和 12 字以内排版校验。

中文字体使用 Source Han Sans SC 子集，共 4 款字号（12 / 14 / 20 / 28）。UI 文案中的非 ASCII 字符由 `tools/collect_glyphs.py` 汇总，`tools/check_font_coverage.py` 在构建前校验字形覆盖。

## 音效

音频驱动复用参考工程，固件内置 16 kHz PCM 音效，运行期不下载任何音频。素材由 CC0 素材包裁剪与程序合成生成，来源和许可见 [assets/audio/SOURCES.md](assets/audio/SOURCES.md)。

| 场景 | 音效 |
| --- | --- |
| 上下切换选项 | `SFX_MOVE` |
| OK 确认 | `SFX_CONFIRM` |
| 答对 | `SFX_BEAM` + `SFX_CORRECT` |
| 答错 / 超时 | `SFX_GROWL` |
| 暂停、受伤、暴击、必杀、胜负 | 对应既有 cue |

音效素材来源与许可见 [assets/audio/SOURCES.md](assets/audio/SOURCES.md)。

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

`build/` 与本地工作记忆 `.ai-memory/` 不纳入版本控制。

## 目录结构

```text
main/
  app/        界面、页面状态与按键分发
  core/       战斗、关卡、题目生成、复习、限时、周目等纯 C 状态机
  audio/      音频播放与生成的音效数据
  assets/     诗词库与像素精灵的生成数据
  fonts/      中文字体子集
assets/
  poems/      诗词 JSON 源文件
  audio/      音频源素材与许可说明
  fonts/      字形清单与字体许可
tests/        可在主机运行的核心逻辑测试
tools/        素材生成、校验与固件归档脚本
```

## 验证状态

最近一次完整构建（2026-11-22）：

```text
Build: PASS
Host tests: NOT RUN
Device tests: NOT RUN
Unverified: 真机显示、按键手感、音量、长时间稳定性
```

构建、合并镜像校验、诗词库、字形清单、字体覆盖、资源与仓库文本检查均通过。主机测试已接入 `tools/validate.sh`，但当前开发机没有主机 C 编译器，只完成了 RISC-V GCC 语法校验，测试断言尚未实际执行，也未进行真机刷写验证。

## 许可与致谢

- 工程框架与 BSP：[FoloToy/ai-passport](https://gitee.com/FoloToy/ai-passport)
- 玩法与渲染参考：[leftstick/ai-passport-match-ultraman](https://github.com/leftstick/ai-passport-match-ultraman)
- 字体：Source Han Sans SC，许可见 `assets/fonts/SourceHanSansSC-LICENSE.txt`
- 音效：Kenney CC0 素材包与 Freesound CC0 录音，详见 `assets/audio/SOURCES.md`
