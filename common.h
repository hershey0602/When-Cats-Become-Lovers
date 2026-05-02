// Common types, globals, utilities, layout

#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <gdiplus.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <cwchar>
#include <cwctype>
#include <vector>

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;
using namespace std;

struct CatState {
    string catName;
    string catBreed;
    string humanName;
    string humanGender;
    string playerPreference;
    string humanPersonality;
    string catTrait;
    int day = 1;
    int affection = 20;
    int trust = 10;
    int mood = 50;
    int energy = 70;
    bool hasToy = false;
    bool hasWarmNest = false;
    bool hasSecret = false;
    bool transformed = false;
};

enum UiPage {
    UiPageStart = 0,
    UiPageCatSelect = 1,
    UiPageProfileSetup = 2,
    UiPageStory = 3,
    UiPageEnding = 4
};

enum StoryNodeType {
    StoryNodeDialog = 0,
    StoryNodeChoice = 1,
    StoryNodeFinish = 2
};

struct StoryChoiceDef {
    const wchar_t* text;
    int nextIndex;
};

struct StoryNode {
    StoryNodeType type;
    const wchar_t* text;
    bool useHumanPortrait;
    int choiceCount;
    StoryChoiceDef choices[3];
};

static const wchar_t* kWindowClass = L"CatGameStartWindow";
static const wchar_t* kWindowTitle = L"养猫恋爱小游戏";
static bool gStartRequested = false;
static ULONG_PTR gGdiToken = 0;
static UiPage gCurrentPage = UiPageStart;
static int gSelectedBreedChoice = 0;
static int gSelectedGenderChoice = 0;
static wstring gSelectedCatName;
static Image* gBackground = nullptr;
static Image* gLivingRoomBackground = nullptr;
static Image* gDialogBoxImage = nullptr;
static Image* gPromptBoxImage = nullptr;
static Image* gCatBlackImage = nullptr;
static Image* gCatCalicoImage = nullptr;
static Image* gCatRagdollImage = nullptr;
static Image* gHumanBlackMaleImage = nullptr;
static Image* gHumanBlackFemaleImage = nullptr;
static Image* gHumanCalicoMaleImage = nullptr;
static Image* gHumanCalicoFemaleImage = nullptr;
static Image* gHumanRagdollMaleImage = nullptr;
static Image* gHumanRagdollFemaleImage = nullptr;
static HWND gNameEdit = nullptr;
static HFONT gUiFont = nullptr;
static HBRUSH gEditBrush = nullptr;
static RECT gTitleRect = { 0, 0, 0, 0 };
static RECT gStartButtonRect = { 0, 0, 0, 0 };
static RECT gBottomDialogRect = { 0, 0, 0, 0 };
static RECT gQuestionRect = { 0, 0, 0, 0 };
static RECT gCatRects[3] = {};
static RECT gGenderQuestionRect = { 0, 0, 0, 0 };
static RECT gGenderButtons[2] = {};
static RECT gNamePromptRect = { 0, 0, 0, 0 };
static RECT gNameInputRect = { 0, 0, 0, 0 };
static RECT gReadyButtonRect = { 0, 0, 0, 0 };
static RECT gPromptBoxRect = { 0, 0, 0, 0 };
static RECT gPromptQuestionRect = { 0, 0, 0, 0 };
static RECT gPromptOptionRects[4] = {};
static RECT gTopRightButtonRect = { 0, 0, 0, 0 };
static RECT gEndingTitleRect = { 0, 0, 0, 0 };
static RECT gEndingButtons[3] = {};
static bool gPromptVisible = false;
static int gPromptOptionCount = 0;
static int gPromptSelectedIndex = -1;
static wstring gPromptQuestion;
static wstring gPromptOptions[4];
static RECT gStoryPortraitRect = { 0, 0, 0, 0 };
static RECT gStoryTextRect = { 0, 0, 0, 0 };
static const StoryNode* gCurrentStoryNodes = nullptr;
static int gCurrentStoryNodeCount = 0;
static int gCurrentStoryNodeIndex = -1;
static wstring gCurrentDialogueText;
static vector<wstring> gCurrentDialoguePages;
static int gCurrentDialoguePageIndex = 0;
static int gVisibleDialogueChars = 0;
static bool gDialogueFullyShown = false;
static wchar_t gCatSelectQuestionText[] = L"请领一只回家吧（不可以全都要哦）";
static vector<int> gChoiceHistory;
static bool gShowStartButton = false;

static HFONT getCachedUiFont(int pixelHeight, int weight = FW_NORMAL) {
    struct FontSlot {
        int size = 0;
        int fontWeight = 0;
        HFONT handle = nullptr;
    };
    static FontSlot slots[8] = {};

    for (FontSlot& slot : slots) {
        if (slot.handle != nullptr && slot.size == pixelHeight && slot.fontWeight == weight) {
            return slot.handle;
        }
    }

    for (FontSlot& slot : slots) {
        if (slot.handle == nullptr) {
            slot.size = pixelHeight;
            slot.fontWeight = weight;
            slot.handle = CreateFontW(
                -pixelHeight,
                0, 0, 0,
                weight,
                FALSE, FALSE, FALSE,
                DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY,
                DEFAULT_PITCH | FF_DONTCARE,
                L"Microsoft YaHei UI"
            );
            return slot.handle;
        }
    }

    return static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
}

static bool pointInRect(int x, int y, const RECT& rc) {
    return x >= rc.left && x <= rc.right && y >= rc.top && y <= rc.bottom;
}

static int clampValue(int x, int lo, int hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

static string utf8FromWide(const wstring& text) {
    if (text.empty()) {
        return "";
    }
    int size = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    string result(size > 0 ? size - 1 : 0, '\0');
    if (size > 1) {
        WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, &result[0], size - 1, nullptr, nullptr);
    }
    return result;
}

static wstring trimWide(const wstring& text) {
    size_t start = 0;
    size_t end = text.size();
    while (start < text.size() && iswspace(text[start])) {
        ++start;
    }
    while (end > start && iswspace(text[end - 1])) {
        --end;
    }
    return text.substr(start, end - start);
}

static wstring stripStoryTags(const wstring& text) {
    wstring result;
    size_t start = 0;
    while (start < text.size()) {
        size_t end = text.find(L'\n', start);
        if (end == wstring::npos) {
            end = text.size();
        }
        wstring line = text.substr(start, end - start);
        if (!(line.size() >= 2 && line.front() == L'【' && line.find(L'】') != wstring::npos)) {
            if (!result.empty() && !line.empty()) {
                result += L"\n";
            }
            result += line;
        }
        start = end + 1;
    }
    return result;
}

static void replaceAllInPlace(wstring& text, const wstring& from, const wstring& to) {
    if (from.empty()) {
        return;
    }
    size_t pos = 0;
    while ((pos = text.find(from, pos)) != wstring::npos) {
        text.replace(pos, from.size(), to);
        pos += to.size();
    }
}

static wstring formatStoryTextForDisplay(const wchar_t* rawText) {
    wstring result = stripStoryTags(rawText != nullptr ? rawText : L"");
    if (!gSelectedCatName.empty()) {
        replaceAllInPlace(result, L"TA", gSelectedCatName);
    }
    return result;
}

static vector<wstring> paginateDialogueText(const wstring& text) {
    const size_t kMaxCharsPerPage = 120;
    vector<wstring> pages;
    if (text.empty()) {
        pages.push_back(L"");
        return pages;
    }

    size_t start = 0;
    while (start < text.size()) {
        size_t remaining = text.size() - start;
        if (remaining <= kMaxCharsPerPage) {
            pages.push_back(text.substr(start));
            break;
        }

        size_t split = start + kMaxCharsPerPage;
        size_t best = split;
        for (size_t i = split; i > start + kMaxCharsPerPage / 2; --i) {
            wchar_t ch = text[i - 1];
            if (ch == L'\n' || ch == L'。' || ch == L'！' || ch == L'？' || ch == L'”') {
                best = i;
                break;
            }
        }

        pages.push_back(text.substr(start, best - start));
        start = best;
        while (start < text.size() && text[start] == L'\n') {
            ++start;
        }
    }

    return pages;
}

static void clearInput() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

static int askChoice(int minOption, int maxOption) {
    int choice;
    while (true) {
        cout << "\n请输入选项编号(" << minOption << "-" << maxOption << "): ";
        if (cin >> choice && choice >= minOption && choice <= maxOption) {
            return choice;
        }
        cout << "输入无效，请重试。\n";
        clearInput();
    }
}

static void setupCatByBreed(CatState& s, int breedChoice) {
    if (breedChoice == 1) {
        s.catBreed = "布偶猫";
        s.humanName = (s.playerPreference == "男") ? "沈绒" : "沈绫";
        s.humanPersonality = "温柔、安静、黏人，擅长察觉你的情绪";
        s.catTrait = "喜欢贴着你坐着，紧张时会轻轻拽你的衣角";
        s.affection = 24;
        s.trust = 14;
        s.mood = 55;
        s.energy = 65;
    } else if (breedChoice == 2) {
        s.catBreed = "橘猫";
        s.humanName = (s.playerPreference == "男") ? "陆阳" : "陆柚";
        s.humanPersonality = "开朗、会逗人开心、有点小贪吃";
        s.catTrait = "看到零食就眼睛发亮，开心时会忍不住围着你转";
        s.affection = 22;
        s.trust = 12;
        s.mood = 60;
        s.energy = 72;
    } else if (breedChoice == 3) {
        s.catBreed = "缅因猫";
        s.humanName = (s.playerPreference == "男") ? "顾凛" : "顾岚";
        s.humanPersonality = "可靠、克制、外冷内热，很有保护欲";
        s.catTrait = "习惯守在门边等你回家，不爱撒娇却总在暗中照顾你";
        s.affection = 18;
        s.trust = 16;
        s.mood = 48;
        s.energy = 68;
    } else {
        s.catBreed = "黑猫";
        s.humanName = (s.playerPreference == "男") ? "夜澈" : "夜璃";
        s.humanPersonality = "神秘、敏锐、嘴硬心软，像藏着秘密";
        s.catTrait = "总爱悄悄观察你，被发现后又装作若无其事";
        s.affection = 20;
        s.trust = 18;
        s.mood = 50;
        s.energy = 70;
    }
    s.humanGender = s.playerPreference;
}

static void printStatus(const CatState& s) {
    cout << "\n========== 当前状态 ==========\n";
    cout << "第 " << s.day << " 天\n";
    cout << "猫咪: " << s.catName << "\n";
    cout << "品种: " << s.catBreed << "\n";
    cout << "好感度: " << s.affection << " / 100\n";
    cout << "信任值: " << s.trust << " / 100\n";
    cout << "心情: " << s.mood << " / 100\n";
    cout << "体力: " << s.energy << " / 100\n";
    cout << "=============================\n";
}

static void applyDelta(CatState& s, int affection, int trust, int mood, int energy) {
    s.affection = clampValue(s.affection + affection, 0, 100);
    s.trust = clampValue(s.trust + trust, 0, 100);
    s.mood = clampValue(s.mood + mood, 0, 100);
    s.energy = clampValue(s.energy + energy, 0, 100);
}

static void openingStory(const CatState& s) {
    cout << "雨夜里，你在巷口捡到一只湿漉漉的小猫。\n";
    cout << "你带回家的是一只【" << s.catBreed << "】。\n";
    cout << "它抬头看着你，眼神像在问：\"可以带我回家吗？\"\n";
    cout << "你给它取名为【" << s.catName << "】。\n";
    cout << "它的性子像是：" << s.humanPersonality << "。\n";
    cout << "接下来的三个夜晚，你们的距离会迅速拉近。\n";
}

static void dailyRandomEvent(CatState& s) {
    int roll = rand() % 100;
    cout << "\n--- 今日小事件 ---\n";

    if (roll < 25) {
        cout << s.catName << "把你的耳机线咬断了，却心虚地把爪子缩起来。\n";
        cout << "你摸了摸它的头，没有责怪。\n";
        applyDelta(s, 4, 4, 3, -2);
    } else if (roll < 50) {
        cout << "清晨，它第一次主动跳到你腿上打盹。\n";
        applyDelta(s, 6, 5, 2, -3);
    } else if (roll < 75) {
        cout << "雷声太大，" << s.catName << "躲在床底不肯出来。\n";
        cout << "你轻声安慰，它慢慢靠近你。\n";
        applyDelta(s, 3, 6, -2, -5);
    } else {
        cout << "它叼来一朵被雨打湿的小花，像是在送你礼物。\n";
        applyDelta(s, 8, 4, 6, -2);
    }
}

static void actionFeed(CatState& s) {
    cout << s.catName << "认真吃完了你准备的晚饭，尾巴轻轻晃着。\n";
    applyDelta(s, 5, 2, 6, 8);
}

static void actionPlay(CatState& s) {
    cout << "你拿逗猫棒和它玩了很久，客厅里全是欢快的脚步声。\n";
    if (!s.hasToy) {
        cout << "你顺手给它买了新的玩具球。\n";
        s.hasToy = true;
        applyDelta(s, 9, 5, 10, -8);
    } else {
        applyDelta(s, 7, 4, 8, -10);
    }
}

static void actionClean(CatState& s) {
    cout << "你把角落整理干净，还给它铺了更软的垫子。\n";
    if (!s.hasWarmNest) {
        cout << "它第一次在你面前翻肚皮睡着了。\n";
        s.hasWarmNest = true;
        applyDelta(s, 8, 8, 5, 4);
    } else {
        applyDelta(s, 4, 6, 3, 5);
    }
}

static void actionTalk(CatState& s) {
    cout << "你对它说起白天的烦恼，它安静听着，偶尔轻声喵一声。\n";
    if (s.trust >= 28 && !s.hasSecret) {
        cout << "它用爪子按住你的手，眼神里像藏着一个秘密。\n";
        s.hasSecret = true;
        applyDelta(s, 10, 12, 4, -2);
    } else {
        applyDelta(s, 6, 7, 2, -1);
    }
}

static void chooseDailyAction(CatState& s) {
    cout << "\n--- 今日互动 ---\n";
    cout << "1. 喂食照顾\n";
    cout << "2. 陪它玩耍\n";
    cout << "3. 整理猫窝\n";
    cout << "4. 和它聊天\n";

    int choice = askChoice(1, 4);
    if (choice == 1) actionFeed(s);
    if (choice == 2) actionPlay(s);
    if (choice == 3) actionClean(s);
    if (choice == 4) actionTalk(s);
}

static void dayEndPenalty(CatState& s) {
    applyDelta(s, 0, 0, -4, -6);
    if (s.energy < 25) {
        cout << "它今天有点没精神，缩在角落发呆。\n";
        applyDelta(s, -2, -1, -3, 0);
    }
}

static bool canTransform(const CatState& s) {
    return s.affection >= 45 && s.trust >= 35;
}

static void ending(CatState& s) {
    cout << "\n========== 结局 ==========\n";
    if (canTransform(s)) {
        s.transformed = true;
        cout << "第三天深夜，月光落在窗边。\n";
        cout << s.catName << "在你面前缓缓化作人形，轻声说：\n";
        cout << "\"谢谢你一直把我当家人。现在，我可以用人的身份留在你身边了。\"\n";
        cout << "你眼前出现的是名叫【" << s.humanName << "】的" << s.humanGender << "性角色。\n";
        cout << "TA给人的感觉是：" << s.humanPersonality << "。\n";
        cout << "即使变成人后，TA还是保留着猫咪习性：" << s.catTrait << "。\n";
        cout << "\n[结局A] 月下初见（人形成功）\n";
    } else if (s.affection >= 55) {
        cout << "它没有变成人，但会在你回家时第一时间跑来迎接。\n";
        cout << "你知道，这段陪伴已经足够珍贵。\n";
        cout << "\n[结局B] 温柔日常（高好感陪伴）\n";
    } else {
        cout << "它仍有些怕生，常常独自望向窗外。\n";
        cout << "故事还没结束，也许下一次你会更懂它。\n";
        cout << "\n[结局C] 未完待续（普通结局）\n";
    }
    cout << "=========================\n";
}

static bool fileExists(const wchar_t* path) {
    DWORD attr = GetFileAttributesW(path);
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

static wstring joinPath(const wstring& base, const wchar_t* tail) {
    if (base.empty()) {
        return wstring(tail);
    }
    if (base.back() == L'\\' || base.back() == L'/') {
        return base + tail;
    }
    return base + L"\\" + tail;
}

static wstring getExeDirectory() {
    wchar_t buffer[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    wchar_t* slash = wcsrchr(buffer, L'\\');
    if (slash != nullptr) {
        *slash = L'\0';
    }
    return wstring(buffer);
}

static wstring findAssetPath(const wchar_t* fileName) {
    const wchar_t* absoluteRoots[] = {
        L"C:\\Users\\hershey\\Desktop\\test\\test\\assets\\",
        L"C:\\Users\\hershey\\Downloads\\"
    };
    for (const wchar_t* root : absoluteRoots) {
        wstring full = wstring(root) + fileName;
        if (fileExists(full.c_str())) {
            return full;
        }
    }

    const wchar_t* prefixes[] = {
        L"assets\\",
        L"..\\assets\\",
        L"..\\..\\assets\\",
        L"..\\..\\..\\assets\\"
    };

    wstring exeDir = getExeDirectory();
    for (const wchar_t* prefix : prefixes) {
        wstring relative = wstring(prefix) + fileName;
        wstring full = joinPath(exeDir, relative.c_str());
        if (fileExists(full.c_str())) {
            return full;
        }
    }
    return L"";
}

static void buildRoundedRect(GraphicsPath& path, const RectF& rect, float radius) {
    float diameter = radius * 2.0f;
    path.Reset();
    path.AddArc(rect.X, rect.Y, diameter, diameter, 180.0f, 90.0f);
    path.AddArc(rect.GetRight() - diameter, rect.Y, diameter, diameter, 270.0f, 90.0f);
    path.AddArc(rect.GetRight() - diameter, rect.GetBottom() - diameter, diameter, diameter, 0.0f, 90.0f);
    path.AddArc(rect.X, rect.GetBottom() - diameter, diameter, diameter, 90.0f, 90.0f);
    path.CloseFigure();
}

static void updateStartLayout(const RECT& clientRect) {
    gTopRightButtonRect.left = clientRect.right - 252;
    gTopRightButtonRect.top = 24;
    gTopRightButtonRect.right = clientRect.right - 28;
    gTopRightButtonRect.bottom = 86;

    int titleWidth = 980;
    int titleHeight = 220;
    int titleLeft = (clientRect.right - titleWidth) / 2;
    int titleTop = clientRect.bottom / 2 - 170;
    gTitleRect.left = titleLeft;
    gTitleRect.top = titleTop;
    gTitleRect.right = titleLeft + titleWidth;
    gTitleRect.bottom = titleTop + titleHeight;

    int buttonWidth = 420;
    int buttonHeight = 118;
    int buttonLeft = (clientRect.right - buttonWidth) / 2;
    int buttonTop = clientRect.bottom / 2 + 120;
    gStartButtonRect.left = buttonLeft;
    gStartButtonRect.top = buttonTop;
    gStartButtonRect.right = buttonLeft + buttonWidth;
    gStartButtonRect.bottom = buttonTop + buttonHeight;
}

static void updateCatSelectLayout(const RECT& clientRect) {
    gTopRightButtonRect.left = clientRect.right - 252;
    gTopRightButtonRect.top = 24;
    gTopRightButtonRect.right = clientRect.right - 28;
    gTopRightButtonRect.bottom = 86;

    gBottomDialogRect.left = 85;
    gBottomDialogRect.right = clientRect.right - 85;
    gBottomDialogRect.bottom = clientRect.bottom - 28;
    gBottomDialogRect.top = clientRect.bottom - 230;

    int questionWidth = 440;
    int questionHeight = 120;
    int questionLeft = (clientRect.right - questionWidth) / 2;
    int questionTop = 110;
    gQuestionRect.left = questionLeft;
    gQuestionRect.top = questionTop;
    gQuestionRect.right = questionLeft + questionWidth;
    gQuestionRect.bottom = questionTop + questionHeight;

    int catTop = 220;
    int catWidth = 248;
    int catHeight = 310;
    int spacing = 56;
    int totalWidth = catWidth * 3 + spacing * 2;
    int startLeft = (clientRect.right - totalWidth) / 2;

    for (int i = 0; i < 3; ++i) {
        gCatRects[i].left = startLeft + i * (catWidth + spacing);
        gCatRects[i].top = catTop;
        gCatRects[i].right = gCatRects[i].left + catWidth;
        gCatRects[i].bottom = gCatRects[i].top + catHeight;
    }
}

static void updateProfileSetupLayout(const RECT& clientRect) {
    gTopRightButtonRect.left = clientRect.right - 252;
    gTopRightButtonRect.top = 24;
    gTopRightButtonRect.right = clientRect.right - 28;
    gTopRightButtonRect.bottom = 86;

    int centerX = clientRect.right / 2;
    gGenderQuestionRect.left = centerX - 380;
    gGenderQuestionRect.top = 96;
    gGenderQuestionRect.right = centerX + 380;
    gGenderQuestionRect.bottom = gGenderQuestionRect.top + 64;

    int buttonWidth = 220;
    int buttonHeight = 84;
    int buttonGap = 56;
    int buttonTop = 182;
    gGenderButtons[0].left = centerX - buttonGap / 2 - buttonWidth;
    gGenderButtons[0].top = buttonTop;
    gGenderButtons[0].right = gGenderButtons[0].left + buttonWidth;
    gGenderButtons[0].bottom = buttonTop + buttonHeight;
    gGenderButtons[1].left = centerX + buttonGap / 2;
    gGenderButtons[1].top = buttonTop;
    gGenderButtons[1].right = gGenderButtons[1].left + buttonWidth;
    gGenderButtons[1].bottom = buttonTop + buttonHeight;

    gNamePromptRect.left = centerX - 430;
    gNamePromptRect.top = 310;
    gNamePromptRect.right = centerX + 430;
    gNamePromptRect.bottom = gNamePromptRect.top + 52;

    gNameInputRect.left = centerX - 250;
    gNameInputRect.top = 392;
    gNameInputRect.right = centerX + 250;
    gNameInputRect.bottom = gNameInputRect.top + 40;

    gReadyButtonRect.left = centerX - 170;
    gReadyButtonRect.top = 480;
    gReadyButtonRect.right = centerX + 170;
    gReadyButtonRect.bottom = gReadyButtonRect.top + 72;
}

static void updateStoryLayout(const RECT& clientRect) {
    gTopRightButtonRect.left = clientRect.right - 252;
    gTopRightButtonRect.top = 24;
    gTopRightButtonRect.right = clientRect.right - 28;
    gTopRightButtonRect.bottom = 86;

    gBottomDialogRect.left = 85;
    gBottomDialogRect.right = clientRect.right - 85;
    gBottomDialogRect.bottom = clientRect.bottom - 16;
    gBottomDialogRect.top = clientRect.bottom - clientRect.bottom / 3 - 34;

    gStoryPortraitRect.left = 120;
    gStoryPortraitRect.top = 70;
    gStoryPortraitRect.right = 560;
    gStoryPortraitRect.bottom = gBottomDialogRect.top + 20;

    gStoryTextRect.left = gBottomDialogRect.left + 168;
    gStoryTextRect.top = gBottomDialogRect.top + 70;
    gStoryTextRect.right = gBottomDialogRect.right - 138;
    gStoryTextRect.bottom = gBottomDialogRect.bottom - 74;
}

static void updateEndingLayout(const RECT& clientRect) {
    gTopRightButtonRect.left = clientRect.right - 252;
    gTopRightButtonRect.top = 24;
    gTopRightButtonRect.right = clientRect.right - 28;
    gTopRightButtonRect.bottom = 86;

    int centerX = clientRect.right / 2;
    gEndingTitleRect.left = centerX - 320;
    gEndingTitleRect.top = 150;
    gEndingTitleRect.right = centerX + 320;
    gEndingTitleRect.bottom = 230;

    int buttonWidth = 280;
    int buttonHeight = 76;
    int gap = 28;
    int top = 300;
    for (int i = 0; i < 3; ++i) {
        gEndingButtons[i].left = centerX - buttonWidth / 2;
        gEndingButtons[i].top = top + i * (buttonHeight + gap);
        gEndingButtons[i].right = centerX + buttonWidth / 2;
        gEndingButtons[i].bottom = gEndingButtons[i].top + buttonHeight;
    }
}

static void updatePromptLayout(const RECT& clientRect) {
    int boxWidth = 620;
    int boxHeight = 620;
    int left = (clientRect.right - boxWidth) / 2;
    int top = (clientRect.bottom - boxHeight) / 2;
    gPromptBoxRect.left = left;
    gPromptBoxRect.top = top;
    gPromptBoxRect.right = left + boxWidth;
    gPromptBoxRect.bottom = top + boxHeight;

    gPromptQuestionRect.left = left + 86;
    gPromptQuestionRect.top = top + 164;
    gPromptQuestionRect.right = left + boxWidth - 58;
    gPromptQuestionRect.bottom = top + 334;

    int optionWidth = boxWidth - 160;
    int optionHeight = 56;
    int optionLeft = left + 80;
    int startTop = top + 330;
    int gap = 16;

    for (int i = 0; i < 4; ++i) {
        gPromptOptionRects[i].left = optionLeft;
        gPromptOptionRects[i].top = startTop + i * (optionHeight + gap);
        gPromptOptionRects[i].right = optionLeft + optionWidth;
        gPromptOptionRects[i].bottom = gPromptOptionRects[i].top + optionHeight;
    }
}

static void drawRoundedPanel(Graphics& g, const RectF& rect, Color topColor, Color bottomColor, Color edgeColor) {
    GraphicsPath panelPath;
    buildRoundedRect(panelPath, rect, 30.0f);

    RectF shadowRect = rect;
    shadowRect.X += 7.0f;
    shadowRect.Y += 8.0f;
    GraphicsPath shadowPath;
    buildRoundedRect(shadowPath, shadowRect, 30.0f);
    SolidBrush shadowBrush(Color(80, 94, 55, 24));
    g.FillPath(&shadowBrush, &shadowPath);

    LinearGradientBrush panelBrush(
        PointF(rect.X, rect.Y),
        PointF(rect.X, rect.GetBottom()),
        topColor,
        bottomColor
    );
    g.FillPath(&panelBrush, &panelPath);

    Pen edgePen(edgeColor, 3.0f);
    edgePen.SetLineJoin(LineJoinRound);
    g.DrawPath(&edgePen, &panelPath);

    RectF glossRect(rect.X + 18.0f, rect.Y + 12.0f, rect.Width - 36.0f, 24.0f);
    GraphicsPath glossPath;
    buildRoundedRect(glossPath, glossRect, 12.0f);
    SolidBrush glossBrush(Color(55, 255, 252, 244));
    g.FillPath(&glossBrush, &glossPath);
}

static void drawBackgroundCover(Graphics& g, Image* image, const RECT& clientRect) {
    if (image == nullptr || image->GetLastStatus() != Ok) {
        LinearGradientBrush fallbackBrush(
            Point(0, 0),
            Point(clientRect.right, clientRect.bottom),
            Color(255, 253, 236, 206),
            Color(255, 240, 204, 145)
        );
        g.FillRectangle(&fallbackBrush, 0, 0, clientRect.right, clientRect.bottom);
        return;
    }

    REAL srcW = static_cast<REAL>(image->GetWidth());
    REAL srcH = static_cast<REAL>(image->GetHeight());
    REAL srcAspect = srcW / srcH;
    REAL destAspect = static_cast<REAL>(clientRect.right) / static_cast<REAL>(clientRect.bottom);

    REAL drawW = srcW;
    REAL drawH = srcH;
    REAL srcX = 0.0f;
    REAL srcY = 0.0f;

    if (srcAspect > destAspect) {
        drawW = srcH * destAspect;
        srcX = (srcW - drawW) / 2.0f;
    } else {
        drawH = srcW / destAspect;
        srcY = (srcH - drawH) / 2.0f;
    }

    g.DrawImage(
        image,
        RectF(0.0f, 0.0f, static_cast<REAL>(clientRect.right), static_cast<REAL>(clientRect.bottom)),
        srcX,
        srcY,
        drawW,
        drawH,
        UnitPixel
    );
}

static void drawSimpleCenterText(Graphics& g, const wchar_t* text, const RectF& rect, REAL size, Color color) {
    FontFamily family(L"Microsoft YaHei UI");
    Font font(&family, size, FontStyleBold, UnitPixel);
    StringFormat format;
    format.SetAlignment(StringAlignmentCenter);
    format.SetLineAlignment(StringAlignmentCenter);
    format.SetFormatFlags(StringFormatFlagsLineLimit);
    SolidBrush brush(color);
    g.DrawString(text, -1, &font, rect, &format, &brush);
}

static void drawBottomDialog(Graphics& g) {
    RectF dialogRect(
        static_cast<REAL>(gBottomDialogRect.left),
        static_cast<REAL>(gBottomDialogRect.top),
        static_cast<REAL>(gBottomDialogRect.right - gBottomDialogRect.left),
        static_cast<REAL>(gBottomDialogRect.bottom - gBottomDialogRect.top)
    );

    if (gDialogBoxImage != nullptr && gDialogBoxImage->GetLastStatus() == Ok) {
        g.DrawImage(gDialogBoxImage, dialogRect);
    } else {
        drawRoundedPanel(
            g,
            dialogRect,
            Color(240, 252, 234, 214),
            Color(246, 243, 222, 194),
            Color(220, 180, 142, 110)
        );
    }
}

static void openPromptModal(const wchar_t* question, const wchar_t* option1,
    const wchar_t* option2 = L"", const wchar_t* option3 = L"", const wchar_t* option4 = L"") {
    gPromptVisible = true;
    gPromptSelectedIndex = -1;
    gPromptQuestion = question != nullptr ? question : L"";
    gPromptOptions[0] = option1 != nullptr ? option1 : L"";
    gPromptOptions[1] = option2 != nullptr ? option2 : L"";
    gPromptOptions[2] = option3 != nullptr ? option3 : L"";
    gPromptOptions[3] = option4 != nullptr ? option4 : L"";
    gPromptOptionCount = 0;
    for (int i = 0; i < 4; ++i) {
        if (!gPromptOptions[i].empty()) {
            ++gPromptOptionCount;
        }
    }
}

static void closePromptModal() {
    gPromptVisible = false;
    gPromptOptionCount = 0;
    gPromptSelectedIndex = -1;
    gPromptQuestion.clear();
    for (int i = 0; i < 4; ++i) {
        gPromptOptions[i].clear();
    }
}

