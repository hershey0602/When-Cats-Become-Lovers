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
    int optionHeight = 58;
    int optionLeft = left + 80;
    int startTop = top + 368;
    int gap = 22;

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

static const StoryNode kRagdollStory[] = {
    { StoryNodeDialog, L"【场景旁白】\n傍晚的时光静谧地流淌着，晚霞映在沙发上。\n一只布偶猫安静地趴在那里，你有些意外，它看见你，只是轻轻眨了眨眼。\n好像早就习惯了这个房间。\n窗大开着，窗纱不时轻轻飘动一下，它，似乎是从那里蹦进来的？\n为什么一切自然得好像它本就该在这里，到底是，谁穿越了？", false, 0, {} },
    { StoryNodeChoice, L"你要怎么接近它？", false, 2, { { L"轻轻摸摸它的头。", 2 }, { L"坐到它旁边，先不打扰它。", 3 } } },
    { StoryNodeDialog, L"【猫咪反应】\n它闭上眼睛，没有躲开。\n你的手指陷进柔软的毛里，它轻轻蹭了蹭你的掌心。", false, 0, {} },
    { StoryNodeDialog, L"【猫咪反应】\n你坐在它旁边，没有伸手。\n它看了你一会儿，慢慢挪过来，把身体贴到你的腿边。\n像是不想打扰你，却又不想离你太远。", false, 0, {} },
    { StoryNodeDialog, L"【场景旁白】\n夜深了，房间安静下来。\n你正准备休息，布偶猫忽然跳上床。\n它没有立刻躺下，而是在你身边绕了一圈，像是在寻找一个被允许的位置。", false, 0, {} },
    { StoryNodeChoice, L"你要怎么回应它？", false, 2, { { L"掀开被子，让它睡在你身边。", 6 }, { L"轻轻把它抱进怀里。", 7 } } },
    { StoryNodeDialog, L"【猫咪反应】\n它小心地钻进来，趴在离你不远不近的位置。\n过了一会儿，它才慢慢靠近，把头贴到你的手边。\n它的呼吸很轻，像怕吵醒你，又像怕你消失。", false, 0, {} },
    { StoryNodeDialog, L"【猫咪反应】\n它被抱起来的时候没有挣扎。\n只是把脸埋进你怀里，尾巴轻轻绕住你的手腕。\n那一瞬间，你觉得自己好像被一只猫安慰了。", false, 0, {} },
    { StoryNodeDialog, L"【场景旁白】\n半夜，你被一阵微弱的光惊醒。\n身边原本柔软温热的一团不见了。\n取而代之的，是一个坐在床边的人影。\n\n【场景旁白】\nTA有一双和布偶猫一样温柔的蓝眼睛。\n月光落在TA身上，像落在一场还没醒来的梦里。\n\n【人形反应】\n“别怕。”\n“是我。”\n“我只是……终于能用人的样子陪你了。”", true, 0, {} },
    { StoryNodeChoice, L"你看着眼前的TA，第一句话想说什么？", true, 2, { { L"你真的是刚才那只猫？", 10 }, { L"你一直都能变成人吗？", 11 } } },
    { StoryNodeDialog, L"【人形反应】\nTA点了点头，耳尖微微泛红。\n“嗯。”\n“你摸过我的头，也抱过我。”\n“所以……你应该认得出来吧？”", true, 0, {} },
    { StoryNodeDialog, L"【人形反应】\nTA轻轻摇头。\n“不是一直。”\n“是最近才开始的。”\n“也许是因为，我太想回应你了。”", true, 0, {} },
    { StoryNodeDialog, L"【场景旁白】\n房间里安静得能听见彼此的呼吸。\nTA没有急着解释，也没有靠得太近。\n只是坐在那里，用一种很温柔、又无法忽视的目光看着你。", true, 0, {} },
    { StoryNodeChoice, L"你想问TA什么？", true, 2, { { L"你为什么想变成人？", 14 }, { L"你会一直留下吗？", 15 } } },
    { StoryNodeDialog, L"【人形反应】\nTA低下眼睛，像是在认真想这个问题。\n“因为猫能做的事情太少了。”\n“你难过的时候，我只能蹭蹭你。”\n“你撑不住的时候，我只能陪你睡一会儿。”\n“可是我想做得更多。”", true, 0, {} },
    { StoryNodeDialog, L"【人形反应】\nTA看向你，没有马上回答。\n过了一会儿，TA轻声说：\n“如果你愿意。”\n“我不会擅自走进你的生活。”\n“但只要你回头，我都会在。”", true, 0, {} },
    { StoryNodeDialog, L"【场景旁白】\n天快亮了。\n窗外的颜色从深蓝慢慢变浅。\nTA坐在你身边，手指停在离你很近的地方，却没有贸然碰你。\n那一点距离，反而让空气变得更加微妙。", true, 0, {} },
    { StoryNodeChoice, L"你要怎么回应这份陪伴？", true, 2, { { L"握住TA的手。", 18 }, { L"没有说话，只是看着TA。", 19 } } },
    { StoryNodeDialog, L"【人形反应】\nTA微微一怔。\n然后很慢、很认真地回握住你。\n不是用力抓住，而像是在确认你真的允许TA靠近。\n“这样……可以吗？”", true, 0, {} },
    { StoryNodeDialog, L"【人形反应】\nTA被你看得有些不自在，却没有移开视线。\n“你不说话的时候，我会猜很多。”\n“所以你最好不要怪我误会。”\nTA声音很轻，眼神却比刚才更近。", true, 0, {} },
    { StoryNodeChoice, L"这个夜晚结束前，你想和TA走向怎样的关系？", true, 3, { { L"希望TA永远陪伴你。", 21 }, { L"希望TA支持你继续生活下去。", 22 }, { L"不把关系说破，停在暧昧的边界。", 23 } } },
    { StoryNodeDialog, L"【结局｜永远陪伴你】\nTA靠近你，把额头轻轻抵在你的肩上。\n“以前我只能在你身边睡着。”\n“现在我想在你醒来的时候也陪着你。”\n“你可以脆弱，可以沉默，可以什么都不解释。”\n“我不会逼你变好，也不会催你快一点。”\n“我只是会一直在。”\n窗外天光微亮，TA握着你的手。\n你忽然觉得，明天好像没有那么难了。", true, 0, {} },
    { StoryNodeDialog, L"【结局｜继续勇敢生活】\nTA轻轻碰了碰你的脸，像过去用猫爪碰你一样。\n“你不要因为有人陪你，就忘记自己也能站起来。”\n“我会在你身边。”\n“但我不是来替你生活的。”\n“我是来陪你继续往前走的。”\nTA笑得很温柔。\n“所以，请继续勇敢地生活下去吧。”\n“哪怕慢一点，也没关系。”", true, 0, {} },
    { StoryNodeDialog, L"【结局｜暧昧开放】\nTA的手停在你的手边。\n没有碰上来，也没有离开。\n“你还怕我吗？”\n你没有立刻回答。\nTA轻轻笑了一下。\n“没关系。”\n“今晚我可以留下。”\n“明天也是。”\n“直到你愿意告诉我，我到底算什么。”\n月光落在你们之间。\n那道距离没有消失。\n但谁都没有后退。", true, 0, {} },
    { StoryNodeFinish, L"", true, 0, {} }
};

static const StoryNode kCalicoStory[] = {
    { StoryNodeDialog, L"【场景旁白】\n午后的阳光铺满地板。一只三花猫安静地趴在那里，你有些意外，它兴奋地向你抬起头。\n窗大开着，窗纱不时轻轻飘动一下，它，似乎是从那里蹦进来的？\n它心安理得地享受着房间，好像本就该在这里，到底是，谁穿越了？", false, 0, {} },
    { StoryNodeChoice, L"你要怎么回应它？", false, 2, { { L"伸手逗它玩。", 2 }, { L"假装没看见它。", 3 } } },
    { StoryNodeDialog, L"【猫咪反应】\n它立刻扑上来，两只前爪抱住你的手腕。\n它没有真的咬你，只是用牙尖轻轻碰了一下。\n然后抬头看你，尾巴晃得非常得意。\n像是在说：抓到了，你现在是我的玩具。", false, 0, {} },
    { StoryNodeDialog, L"【猫咪反应】\n它震惊地看着你。\n然后绕到你脚边，啪叽一下倒下，露出肚皮。\n那表情不像猫，更像一个正在夸张表演“你怎么可以不理我”的小演员。", false, 0, {} },
    { StoryNodeDialog, L"【场景旁白】\n它已经完全把你当成了今天的主要活动。\n你走到哪里，它跟到哪里。\n你停下，它就假装自己只是路过。\n你一转头，它又立刻用无辜的眼神看向别处。", false, 0, {} },
    { StoryNodeChoice, L"它又缠上来了，你要怎么办？", false, 2, { { L"把它抱起来。", 6 }, { L"认真和它说话。", 7 } } },
    { StoryNodeDialog, L"【猫咪反应】\n它在你怀里象征性挣扎了两下。\n然后迅速找到最舒服的位置，把下巴搁在你肩上。\n明明是它自己缠上来的，现在却像是你主动舍不得放开它。", false, 0, {} },
    { StoryNodeDialog, L"【猫咪反应】\n你认真地对它说话。\n它也非常认真地“喵喵”回应。\n你说一句，它回一句。\n你停下来，它还不满地叫了一声。\n你忽然有种错觉：它不但听懂了，而且嫌你表达能力太差。", false, 0, {} },
    { StoryNodeDialog, L"【场景旁白】\n夜里，它忽然从你怀里跳下去。\n你以为它又发现了不存在的虫子。\n可下一秒，它身上亮起一阵暖色的光。\n\n【场景旁白】\n猫的影子被拉长，尾巴和耳朵逐渐隐入光里。\n等光散去时，一个人站在你面前。\n\n【人形反应】\nTA低头看了看自己的手，又看了看你。\n然后眼睛一亮。\n“哇。”\n“这样是不是更方便黏着你了？”", true, 0, {} },
    { StoryNodeChoice, L"你看着眼前的TA，第一反应是？", true, 2, { { L"你是刚才那只猫？", 10 }, { L"你先离我远一点。", 11 } } },
    { StoryNodeDialog, L"【人形反应】\n“对啊！”\nTA回答得飞快，还非常骄傲。\n“你居然认出来了。”\n“我还以为你会吓到钻进被子里。”", true, 0, {} },
    { StoryNodeDialog, L"【人形反应】\nTA认真地后退了半步。\n“这样？”\n还没等你回答，TA又凑了回来。\n“不行。”\n“太远了，我不喜欢。”", true, 0, {} },
    { StoryNodeDialog, L"【场景旁白】\nTA似乎完全没有“刚变成人应该紧张”的自觉。\nTA摸摸自己的头发，看看自己的手指，又凑近观察你的表情。\n比起害怕，TA更像是在兴奋地探索新世界。", true, 0, {} },
    { StoryNodeChoice, L"TA突然问你：“我现在看起来怎么样？”", true, 2, { { L"很可爱。", 14 }, { L"有点麻烦。", 15 } } },
    { StoryNodeDialog, L"【人形反应】\nTA明显愣了一下。\n刚才还吵吵闹闹的人，忽然安静了半秒。\n然后TA别过脸，假装很镇定。\n“我当然知道。”\n“但是你可以多说几遍。”", true, 0, {} },
    { StoryNodeDialog, L"【人形反应】\nTA睁大眼睛，一脸不可置信。\n“我都变成人了，你第一反应居然是麻烦？”\nTA抱起手臂，假装生气。\n“那我以后要变得更麻烦一点。”\n“麻烦到你离不开我。”", true, 0, {} },
    { StoryNodeDialog, L"【场景旁白】\n闹了一会儿后，房间忽然安静下来。\nTA坐在你旁边，脚尖轻轻晃着。\n这一次，TA没有立刻开玩笑。\n\n【人形反应】\n“其实我以前一直在看你。”\n“你一个人吃饭的时候。”\n“你很累，但还说没事的时候。”\n“你对我笑，可是眼睛一点也不开心的时候。”", true, 0, {} },
    { StoryNodeChoice, L"你要怎么回应TA？", true, 2, { { L"所以你才变成人？", 18 }, { L"你看得也太多了。", 19 } } },
    { StoryNodeDialog, L"【人形反应】\nTA没有立刻回答。\n只是轻轻碰了碰你的手指。\n这一次不像猫爪那样莽撞，而是小心翼翼的。\n“也许吧。”\n“猫能陪你的时间太短了。”\n“我有点贪心。”\n“想陪得久一点。”", true, 0, {} },
    { StoryNodeDialog, L"【人形反应】\nTA笑了一下。\n“对啊。”\n“所以你以后别想骗我。”\nTA靠近了一点，声音放轻。\n“你开心是真的，还是装的。”\n“我都看得出来。”", true, 0, {} },
    { StoryNodeChoice, L"这个夜晚结束前，你想和TA走向怎样的关系？", true, 3, { { L"希望TA永远陪伴你。", 21 }, { L"希望TA支持你继续生活下去。", 22 }, { L"不把关系说破，停在暧昧的边界。", 23 } } },
    { StoryNodeDialog, L"【结局｜永远陪伴你】\nTA把额头轻轻抵在你的肩上。\n刚才那个吵闹得像夏天汽水的人，此刻安静得不像话。\n“我不会让你一个人待太久的。”\n“你难过，我就吵你。”\n“你不想说话，我就陪你坐着。”\n“你忘记怎么笑，我就每天做很笨的事给你看。”\nTA抬头看你，眼睛亮亮的。\n“反正我已经决定了。”\n“以后你的生活里，要有我。”", true, 0, {} },
    { StoryNodeDialog, L"【结局｜继续勇敢生活】\nTA坐在窗边，晃着腿，看起来还是没心没肺。\n可说出口的话，却比平时认真很多。\n“我最喜欢你的地方，不是你会照顾我。”\n“是你明明很累，还是会继续把日子过下去。”\nTA回头看你。\n“所以继续吧。”\n“继续吃饭，继续出门，继续做你想做又不敢做的事。”\n“我会在旁边闹你、烦你、提醒你。”\nTA伸出小指。\n“约好了。”\n“你继续勇敢生活。”\n“我继续喜欢这样的你。”", true, 0, {} },
    { StoryNodeDialog, L"【结局｜暧昧开放】\nTA忽然凑得很近。\n近到你能看清TA睫毛下的光，也能感觉到那一点故意不退开的呼吸。\n“你是不是在想，我现在到底算什么？”\nTA笑起来。\n“你可以慢慢想。”\n“但是明天醒来，如果你还让我留下——”\nTA轻轻勾住你的袖口。\n“我就当你也有一点喜欢我。”\n灯没有关。\n你们也没有把话说完。\n但有些关系，本来就是从一句没说出口的话开始的。", true, 0, {} },
    { StoryNodeFinish, L"", true, 0, {} }
};

static const StoryNode kBlackStory[] = {
    { StoryNodeDialog, L"【场景旁白】\n夜晚的窗边有一只黑猫。\n它坐在那里，一动不动，像一块被月光切出来的影子。\n你看见它的时候，它没有回头。\n但你很确定，它早就知道你来了。", false, 0, {} },
    { StoryNodeChoice, L"你要怎么接近它？", false, 2, { { L"轻声叫它。", 2 }, { L"坐在不远处陪它。", 3 } } },
    { StoryNodeDialog, L"【猫咪反应】\n它慢慢转过头，看了你一眼。\n那一眼很短，像只是确认你的存在。\n然后它又把头转向窗外。\n但尾巴轻轻动了一下。\n你不知道这算回应，还是警告。", false, 0, {} },
    { StoryNodeDialog, L"【猫咪反应】\n你坐在不远处，没有靠近。\n它没有躲，也没有走。\n你们之间隔着一段安静的距离。\n过了一会儿，它把尾巴收回身边，给你让出了一点位置。\n这大概是黑猫式的允许。", false, 0, {} },
    { StoryNodeDialog, L"【场景旁白】\n它还是不怎么理你。\n但奇怪的是，每次你以为它要离开，它都会停下来。\n像是在等你，又不愿意承认。", false, 0, {} },
    { StoryNodeChoice, L"你想进一步靠近它吗？", false, 2, { { L"试着摸它。", 6 }, { L"把手放在旁边，让它自己决定。", 7 } } },
    { StoryNodeDialog, L"【猫咪反应】\n你的手刚伸过去，它就偏头躲开了。\n动作很快，几乎不给你碰到的机会。\n可它没有走。\n它只是抬眼看你。\n像是在说：还没到时候。", false, 0, {} },
    { StoryNodeDialog, L"【猫咪反应】\n它盯着你的手看了很久。\n久到你以为它不会有任何反应。\n然后，它低下头，用鼻尖极轻地碰了一下你的指节。\n只一下。\n像一次秘密签名。", false, 0, {} },
    { StoryNodeDialog, L"【场景旁白】\n深夜，窗外下起了雨。\n雨声很轻，像有人在玻璃上写字。\n你醒来的时候，窗边不再有猫。\n\n【场景旁白】\n那里站着一个人。\nTA背对着你，黑色的发尾落在肩上。\n月光从侧面照过来，勾出一条冷淡又漂亮的轮廓。\n\n【人形反应】\n“终于醒了。”\n\n【场景旁白】\nTA转过头。\n那双眼睛，和那只黑猫一模一样。", true, 0, {} },
    { StoryNodeChoice, L"你要说什么？", true, 2, { { L"是你？", 10 }, { L"你一直在这里？", 11 } } },
    { StoryNodeDialog, L"【人形反应】\nTA看着你，像是对这个问题不太满意。\n“你现在才确认？”\n语气冷淡。\n但TA没有否认。\n甚至还往你这边多看了一眼。", true, 0, {} },
    { StoryNodeDialog, L"【人形反应】\n“嗯。”\nTA说得很平静。\n“你睡着的时候，防备心比醒着还低。”\n这句话听起来像嘲讽。\n但你忽然意识到，TA可能守了一整夜。", true, 0, {} },
    { StoryNodeDialog, L"【场景旁白】\nTA站在窗边，没有靠近。\n雨声落在你们之间，像一层薄薄的隔断。\nTA明明已经变成了人，却依旧保持着猫一样的距离感。", true, 0, {} },
    { StoryNodeChoice, L"你想让TA靠近一点吗？", true, 2, { { L"过来。", 14 }, { L"你可以不用站那么远。", 15 } } },
    { StoryNodeDialog, L"【人形反应】\nTA挑了一下眉。\n“你在命令我？”\n话是这么说，脚步却已经动了。\nTA走到离你一步远的位置停下。\n刚好是一个让人紧张、却还不能说越界的距离。", true, 0, {} },
    { StoryNodeDialog, L"【人形反应】\nTA沉默了一会儿。\n然后慢慢走近。\n“我以为你会怕。”\n声音很低，像是不想让你听见这句话里的在意。\n但你听见了。", true, 0, {} },
    { StoryNodeDialog, L"【场景旁白】\nTA在你身边坐下。\n没有碰你。\n但存在感比触碰更明显。\n你甚至能感觉到TA的视线落在你身上，不急，不退，也不解释。\n\n【人形反应】\n“你总是这样。”\n“不拒绝，也不靠近。”\n“像是在等谁先越过界线。”", true, 0, {} },
    { StoryNodeChoice, L"你要回应TA吗？", true, 2, { { L"那你想越过吗？", 18 }, { L"你也没有靠近。", 19 } } },
    { StoryNodeDialog, L"【人形反应】\nTA看着你，没有立刻回答。\n那双眼睛太安静，反而让空气一点点变热。\n“我如果想，你拦得住吗？”\nTA说完，却只是抬手，替你整理了一下肩上的衣料。\n指尖很快离开。\n克制得几乎残忍。", true, 0, {} },
    { StoryNodeDialog, L"【人形反应】\nTA低低笑了一声。\n“因为我比你有耐心。”\nTA的目光落在你脸上。\n像一只黑猫盯住了自己的猎物，却并不急着扑上来。\n“不过耐心也不是无限的。”", true, 0, {} },
    { StoryNodeChoice, L"这个夜晚结束前，你想和TA走向怎样的关系？", true, 3, { { L"希望TA永远陪伴你。", 21 }, { L"希望TA支持你继续生活下去。", 22 }, { L"不把关系说破，停在暧昧的边界。", 23 } } },
    { StoryNodeDialog, L"【结局｜永远陪伴你】\n雨停了。\n天还没有亮。\nTA站在门边，像是随时可以离开。\n可手却迟迟没有碰到门把。\n“我不擅长说好听的话。”\n“也不会像温顺的猫一样整天围着你转。”\nTA回头看你，眼神冷得很稳定。\n“但你回头的时候，我会在。”\n“你摔倒的时候，我会拉你。”\n“你想放弃的时候，我会骂醒你。”\n“如果这也算陪伴——”\nTA停了一下。\n“那我会陪你很久。”\n“久到你不再怀疑自己值得被留下。”\nTA没有拥抱你。\n只是重新坐回你身边。\n仿佛离开从来不是一个真正的选项。", true, 0, {} },
    { StoryNodeDialog, L"【结局｜继续勇敢生活】\n窗外的天色一点点变浅。\nTA靠在窗边，像黑夜最后留下的一道影子。\n“你不用变得完美。”\n“不用每次都赢。”\n“也不用假装自己从来不害怕。”\nTA看向你。\n那不是温柔的安慰，更像一种冷静的相信。\n“但你要继续活。”\n“继续走。”\n“继续把那些让你痛苦的日子，一天一天踩过去。”\nTA向你走近一步。\n“我会看着你。”\n“不是监督。”\n“是证明。”\n“证明你没有被那些黑夜吞掉。”\n天亮之前，TA轻轻碰了一下你的手。\n很短。\n像一句没有说出口的：别怕。", true, 0, {} },
    { StoryNodeDialog, L"【结局｜暧昧开放】\n你问TA，明天还会不会在。\nTA没有回答。\nTA只是靠近你，停在一个危险又礼貌的距离。\n近一步太亲密。\n退一步又像逃。\n“你希望我在吗？”\n你没有立刻回答。\nTA低下头，靠近你的耳边。\n声音轻得像夜色本身。\n“门我不会锁。”\n“窗也会留一条缝。”\n“你可以当我离开了。”\n“也可以当我只是暂时藏起来。”\n第二天清晨，窗边没有人。\n只有一根黑色的猫毛落在枕边。\n你不知道TA是否真的离开。\n但从那以后，每个下雨的夜晚，你都会下意识看向窗边。", true, 0, {} },
    { StoryNodeFinish, L"", true, 0, {} }
};

static Image* getCurrentCatImage() {
    if (gSelectedBreedChoice == 1) return gCatRagdollImage;
    if (gSelectedBreedChoice == 2) return gCatCalicoImage;
    return gCatBlackImage;
}

static Image* getCurrentHumanImage() {
    if (gSelectedBreedChoice == 1) {
        return gSelectedGenderChoice == 2 ? gHumanRagdollFemaleImage : gHumanRagdollMaleImage;
    }
    if (gSelectedBreedChoice == 2) {
        return gSelectedGenderChoice == 2 ? gHumanCalicoFemaleImage : gHumanCalicoMaleImage;
    }
    return gSelectedGenderChoice == 2 ? gHumanBlackFemaleImage : gHumanBlackMaleImage;
}

static int getNextStoryIndexForCurrentStory(int currentIndex) {
    if (gCurrentStoryNodes == kRagdollStory || gCurrentStoryNodes == kCalicoStory || gCurrentStoryNodes == kBlackStory) {
        switch (currentIndex) {
        case 2:
        case 3:
            return 4;
        case 6:
        case 7:
            return 8;
        case 10:
        case 11:
            return 12;
        case 14:
        case 15:
            return 16;
        case 18:
        case 19:
            return 20;
        case 21:
        case 22:
        case 23:
            return 24;
        default:
            break;
        }
    }
    return currentIndex + 1;
}

static void showDialogueNode(const StoryNode& node) {
    gCurrentDialogueText = formatStoryTextForDisplay(node.text);
    gCurrentDialoguePages = paginateDialogueText(gCurrentDialogueText);
    gCurrentDialoguePageIndex = 0;
    if (!gCurrentDialoguePages.empty()) {
        gCurrentDialogueText = gCurrentDialoguePages[0];
    }
    gVisibleDialogueChars = 0;
    gDialogueFullyShown = false;
}

static void syncPageControls(HWND hwnd);
static void enterStoryNode(HWND hwnd, int index);

static void startStory(HWND hwnd) {
    gChoiceHistory.clear();
    if (gSelectedBreedChoice == 1) {
        gCurrentStoryNodes = kRagdollStory;
        gCurrentStoryNodeCount = static_cast<int>(sizeof(kRagdollStory) / sizeof(kRagdollStory[0]));
    } else if (gSelectedBreedChoice == 2) {
        gCurrentStoryNodes = kCalicoStory;
        gCurrentStoryNodeCount = static_cast<int>(sizeof(kCalicoStory) / sizeof(kCalicoStory[0]));
    } else {
        gCurrentStoryNodes = kBlackStory;
        gCurrentStoryNodeCount = static_cast<int>(sizeof(kBlackStory) / sizeof(kBlackStory[0]));
    }

    gCurrentPage = UiPageStory;
    ShowWindow(gNameEdit, SW_HIDE);
    enterStoryNode(hwnd, 0);
}

static void resetToStart(HWND hwnd) {
    gSelectedBreedChoice = 0;
    gSelectedGenderChoice = 0;
    gSelectedCatName.clear();
    gCurrentStoryNodes = nullptr;
    gCurrentStoryNodeCount = 0;
    gCurrentStoryNodeIndex = -1;
    gCurrentDialogueText.clear();
    gCurrentDialoguePages.clear();
    gCurrentDialoguePageIndex = 0;
    gChoiceHistory.clear();
    gVisibleDialogueChars = 0;
    gDialogueFullyShown = false;
    gShowStartButton = false;
    closePromptModal();
    KillTimer(hwnd, 2);
    KillTimer(hwnd, 1);
    if (gNameEdit != nullptr) {
        SetWindowTextW(gNameEdit, L"");
    }
    gCurrentPage = UiPageStart;
    syncPageControls(hwnd);
    SetTimer(hwnd, 2, 1000, nullptr);
    InvalidateRect(hwnd, nullptr, FALSE);
}

static void restartGameFlow(HWND hwnd) {
    gSelectedBreedChoice = 0;
    gSelectedGenderChoice = 0;
    gSelectedCatName.clear();
    gCurrentStoryNodes = nullptr;
    gCurrentStoryNodeCount = 0;
    gCurrentStoryNodeIndex = -1;
    gCurrentDialogueText.clear();
    gCurrentDialoguePages.clear();
    gCurrentDialoguePageIndex = 0;
    gChoiceHistory.clear();
    gVisibleDialogueChars = 0;
    gDialogueFullyShown = false;
    gShowStartButton = false;
    closePromptModal();
    KillTimer(hwnd, 2);
    KillTimer(hwnd, 1);
    if (gNameEdit != nullptr) {
        SetWindowTextW(gNameEdit, L"");
    }
    gCurrentPage = UiPageCatSelect;
    syncPageControls(hwnd);
    InvalidateRect(hwnd, nullptr, FALSE);
}

static void enterStoryNode(HWND hwnd, int index) {
    if (gCurrentStoryNodes == nullptr || index < 0 || index >= gCurrentStoryNodeCount) {
        gCurrentPage = UiPageEnding;
        syncPageControls(hwnd);
        InvalidateRect(hwnd, nullptr, FALSE);
        return;
    }

    gCurrentStoryNodeIndex = index;
    const StoryNode& node = gCurrentStoryNodes[index];
    if (node.type == StoryNodeDialog) {
        closePromptModal();
        showDialogueNode(node);
        SetTimer(hwnd, 1, 18, nullptr);
    } else if (node.type == StoryNodeChoice) {
        KillTimer(hwnd, 1);
        openPromptModal(
            node.text,
            node.choiceCount > 0 ? node.choices[0].text : L"",
            node.choiceCount > 1 ? node.choices[1].text : L"",
            node.choiceCount > 2 ? node.choices[2].text : L""
        );
        gCurrentDialogueText.clear();
        gCurrentDialoguePages.clear();
        gCurrentDialoguePageIndex = 0;
        gVisibleDialogueChars = 0;
        gDialogueFullyShown = true;
    } else {
        gCurrentPage = UiPageEnding;
        syncPageControls(hwnd);
        InvalidateRect(hwnd, nullptr, FALSE);
        return;
    }
    InvalidateRect(hwnd, nullptr, TRUE);
}

static void advanceStory(HWND hwnd) {
    if (gCurrentDialoguePageIndex + 1 < static_cast<int>(gCurrentDialoguePages.size())) {
        ++gCurrentDialoguePageIndex;
        gCurrentDialogueText = gCurrentDialoguePages[gCurrentDialoguePageIndex];
        gVisibleDialogueChars = 0;
        gDialogueFullyShown = false;
        SetTimer(hwnd, 1, 18, nullptr);
        InvalidateRect(hwnd, nullptr, FALSE);
        return;
    }
    if (gCurrentStoryNodes == nullptr || gCurrentStoryNodeIndex < 0) {
        return;
    }
    int nextIndex = getNextStoryIndexForCurrentStory(gCurrentStoryNodeIndex);
    if (nextIndex >= gCurrentStoryNodeCount) {
        DestroyWindow(hwnd);
        return;
    }
    enterStoryNode(hwnd, nextIndex);
}

static void returnToPreviousChoice(HWND hwnd) {
    closePromptModal();
    KillTimer(hwnd, 1);

    if (!gChoiceHistory.empty()) {
        int previousChoiceNode = gChoiceHistory.back();
        gChoiceHistory.pop_back();
        enterStoryNode(hwnd, previousChoiceNode);
        return;
    }

    gCurrentStoryNodes = nullptr;
    gCurrentStoryNodeCount = 0;
    gCurrentStoryNodeIndex = -1;
    gCurrentDialogueText.clear();
    gCurrentDialoguePages.clear();
    gCurrentDialoguePageIndex = 0;
    gVisibleDialogueChars = 0;
    gDialogueFullyShown = false;
    gSelectedBreedChoice = 0;
    gSelectedGenderChoice = 0;
    gSelectedCatName.clear();
    if (gNameEdit != nullptr) {
        SetWindowTextW(gNameEdit, L"");
    }
    gCurrentPage = UiPageCatSelect;
    syncPageControls(hwnd);
    InvalidateRect(hwnd, nullptr, FALSE);
}

static void handlePromptSelection(HWND hwnd, int selectedIndex) {
    if (gCurrentStoryNodes == nullptr || gCurrentStoryNodeIndex < 0) {
        return;
    }
    const StoryNode& node = gCurrentStoryNodes[gCurrentStoryNodeIndex];
    if (node.type != StoryNodeChoice) {
        return;
    }
    if (selectedIndex < 0 || selectedIndex >= node.choiceCount) {
        return;
    }
    gChoiceHistory.push_back(gCurrentStoryNodeIndex);
    closePromptModal();
    enterStoryNode(hwnd, node.choices[selectedIndex].nextIndex);
}

static void drawTitleText(Graphics& g, const RectF& rect) {
    FontFamily titleFamily(L"STXingkai");
    StringFormat centered;
    centered.SetAlignment(StringAlignmentCenter);
    centered.SetLineAlignment(StringAlignmentCenter);

    RectF deepShadowRect = rect;
    deepShadowRect.X += 10.0f;
    deepShadowRect.Y += 12.0f;
    GraphicsPath deepShadow;
    deepShadow.AddString(L"猫咪不可以变成妻子除非...", -1, &titleFamily, FontStyleRegular, 66.0f, deepShadowRect, &centered);
    SolidBrush deepShadowBrush(Color(135, 88, 41, 21));
    g.FillPath(&deepShadowBrush, &deepShadow);

    RectF midShadowRect = rect;
    midShadowRect.X += 5.0f;
    midShadowRect.Y += 6.0f;
    GraphicsPath midShadow;
    midShadow.AddString(L"猫咪不可以变成妻子除非...", -1, &titleFamily, FontStyleRegular, 66.0f, midShadowRect, &centered);
    SolidBrush midShadowBrush(Color(120, 125, 62, 30));
    g.FillPath(&midShadowBrush, &midShadow);

    GraphicsPath titlePath;
    titlePath.AddString(L"猫咪不可以变成妻子除非...", -1, &titleFamily, FontStyleRegular, 66.0f, rect, &centered);
    Pen outlinePen(Color(255, 108, 52, 32), 6.0f);
    outlinePen.SetLineJoin(LineJoinRound);
    LinearGradientBrush fillBrush(
        PointF(rect.X, rect.Y),
        PointF(rect.X, rect.GetBottom()),
        Color(255, 255, 252, 245),
        Color(255, 252, 221, 187)
    );
    g.DrawPath(&outlinePen, &titlePath);
    g.FillPath(&fillBrush, &titlePath);

    FontFamily heartFamily(L"Microsoft YaHei UI");
    GraphicsPath leftHeart;
    leftHeart.AddString(L"♡", -1, &heartFamily, FontStyleBold, 36.0f,
        PointF(rect.X + 40.0f, rect.Y + 70.0f), &centered);
    GraphicsPath rightHeart;
    rightHeart.AddString(L"♡", -1, &heartFamily, FontStyleBold, 36.0f,
        PointF(rect.GetRight() - 40.0f, rect.Y + 70.0f), &centered);
    SolidBrush heartBrush(Color(255, 240, 123, 131));
    Pen heartOutline(Color(220, 126, 59, 56), 2.0f);
    g.DrawPath(&heartOutline, &leftHeart);
    g.FillPath(&heartBrush, &leftHeart);
    g.DrawPath(&heartOutline, &rightHeart);
    g.FillPath(&heartBrush, &rightHeart);
}

static void drawLoadFailText(Graphics& g, const wchar_t* text, const RectF& rect);
static void drawCenteredText(Graphics& g, const wchar_t* text, const RectF& rect, REAL size, Color fill, Color outline);

static void drawStartTitle(Graphics& g) {
    RectF titleRect(
        static_cast<REAL>(gTitleRect.left),
        static_cast<REAL>(gTitleRect.top),
        static_cast<REAL>(gTitleRect.right - gTitleRect.left),
        static_cast<REAL>(gTitleRect.bottom - gTitleRect.top)
    );
    drawTitleText(g, titleRect);
}

static void drawStartButton(Graphics& g) {
    RectF buttonRect(
        static_cast<REAL>(gStartButtonRect.left),
        static_cast<REAL>(gStartButtonRect.top),
        static_cast<REAL>(gStartButtonRect.right - gStartButtonRect.left),
        static_cast<REAL>(gStartButtonRect.bottom - gStartButtonRect.top)
    );

    drawRoundedPanel(
        g,
        buttonRect,
        Color(248, 255, 230, 244),
        Color(252, 244, 171, 221),
        Color(238, 166, 96, 170)
    );

    FontFamily family(L"Microsoft YaHei UI");
    StringFormat format;
    format.SetAlignment(StringAlignmentCenter);
    format.SetLineAlignment(StringAlignmentCenter);

    GraphicsPath highlightPath;
    highlightPath.AddString(L"开始游戏", -1, &family, FontStyleBold, 38.0f,
        RectF(buttonRect.X, buttonRect.Y - 2.0f, buttonRect.Width, buttonRect.Height), &format);
    SolidBrush highlightBrush(Color(140, 255, 245, 252));
    g.FillPath(&highlightBrush, &highlightPath);

    GraphicsPath shadowPath;
    shadowPath.AddString(L"开始游戏", -1, &family, FontStyleBold, 38.0f,
        RectF(buttonRect.X + 3.0f, buttonRect.Y + 6.0f, buttonRect.Width, buttonRect.Height), &format);
    SolidBrush shadowBrush(Color(110, 92, 58, 126));
    g.FillPath(&shadowBrush, &shadowPath);

    GraphicsPath textPath;
    textPath.AddString(L"开始游戏", -1, &family, FontStyleBold, 38.0f, buttonRect, &format);
    Pen outlinePen(Color(235, 143, 95, 163), 4.0f);
    outlinePen.SetLineJoin(LineJoinRound);
    SolidBrush fillBrush(Color(255, 110, 72, 150));
    g.DrawPath(&outlinePen, &textPath);
    g.FillPath(&fillBrush, &textPath);
}

static void drawTopRightHomeButton(Graphics& g) {
    RectF buttonRect(
        static_cast<REAL>(gTopRightButtonRect.left),
        static_cast<REAL>(gTopRightButtonRect.top),
        static_cast<REAL>(gTopRightButtonRect.right - gTopRightButtonRect.left),
        static_cast<REAL>(gTopRightButtonRect.bottom - gTopRightButtonRect.top)
    );

    drawRoundedPanel(
        g,
        buttonRect,
        Color(238, 255, 248, 240),
        Color(246, 244, 223, 196),
        Color(230, 188, 150, 126)
    );

    RectF glossRect(buttonRect.X + 12.0f, buttonRect.Y + 8.0f, buttonRect.Width - 24.0f, 16.0f);
    GraphicsPath glossPath;
    buildRoundedRect(glossPath, glossRect, 8.0f);
    SolidBrush glossBrush(Color(72, 255, 255, 252));
    g.FillPath(&glossBrush, &glossPath);

    drawSimpleCenterText(g, L"返回上一个选项", buttonRect, 18.0f, Color(255, 98, 74, 64));
}

static void drawQuestionPanel(Graphics& g) {
    RectF panelRect(
        static_cast<REAL>(gQuestionRect.left),
        static_cast<REAL>(gQuestionRect.top),
        static_cast<REAL>(gQuestionRect.right - gQuestionRect.left),
        static_cast<REAL>(gQuestionRect.bottom - gQuestionRect.top)
    );

    FontFamily family(L"Microsoft YaHei UI");
    StringFormat format;
    format.SetAlignment(StringAlignmentCenter);
    format.SetLineAlignment(StringAlignmentCenter);
    format.SetFormatFlags(StringFormatFlagsLineLimit);

    GraphicsPath textShadow;
    textShadow.AddString(L"请领一只回家吧\n（不可以全都要哦）", -1, &family, FontStyleBold, 20.0f,
        RectF(panelRect.X + 2.0f, panelRect.Y + 4.0f, panelRect.Width, panelRect.Height), &format);
    SolidBrush shadowBrush(Color(95, 35, 30, 28));
    g.FillPath(&shadowBrush, &textShadow);

    GraphicsPath textPath;
    textPath.AddString(L"请领一只回家吧\n（不可以全都要哦）", -1, &family, FontStyleBold, 20.0f,
        panelRect, &format);
    Pen outlinePen(Color(220, 34, 30, 28), 2.2f);
    outlinePen.SetLineJoin(LineJoinRound);
    SolidBrush fillBrush(Color(255, 247, 234, 188));
    g.DrawPath(&outlinePen, &textPath);
    g.FillPath(&fillBrush, &textPath);
}

static void drawImageFitCropped(Graphics& g, Image* image, const RectF& destRect) {
    if (image == nullptr || image->GetLastStatus() != Ok) {
        return;
    }

    REAL srcW = static_cast<REAL>(image->GetWidth());
    REAL srcH = static_cast<REAL>(image->GetHeight());

    REAL cropLeft = srcW * 0.09f;
    REAL cropTop = srcH * 0.05f;
    REAL cropRightTrim = srcW * 0.09f;
    REAL cropBottomTrim = srcH * 0.11f;

    if (image == gCatBlackImage) {
        cropLeft = srcW * 0.06f;
        cropTop = srcH * 0.03f;
        cropRightTrim = srcW * 0.06f;
        cropBottomTrim = srcH * 0.28f;
    }

    REAL cropWidth = srcW - cropLeft - cropRightTrim;
    REAL cropHeight = srcH - cropTop - cropBottomTrim;

    RectF srcRect(cropLeft, cropTop, cropWidth, cropHeight);
    REAL srcAspect = srcRect.Width / srcRect.Height;
    REAL destAspect = destRect.Width / destRect.Height;

    RectF drawRect = destRect;
    if (srcAspect > destAspect) {
        REAL fittedHeight = destRect.Width / srcAspect;
        drawRect.Y += (destRect.Height - fittedHeight) / 2.0f;
        drawRect.Height = fittedHeight;
    } else {
        REAL fittedWidth = destRect.Height * srcAspect;
        drawRect.X += (destRect.Width - fittedWidth) / 2.0f;
        drawRect.Width = fittedWidth;
    }

    g.DrawImage(image, drawRect, srcRect.X, srcRect.Y, srcRect.Width, srcRect.Height, UnitPixel);
}

static void drawCatCard(Graphics& g, const RECT& rect, Image* image) {
    RectF cardRect(
        static_cast<REAL>(rect.left),
        static_cast<REAL>(rect.top),
        static_cast<REAL>(rect.right - rect.left),
        static_cast<REAL>(rect.bottom - rect.top)
    );

    drawRoundedPanel(
        g,
        cardRect,
        Color(210, 255, 241, 218),
        Color(215, 246, 205, 151),
        Color(220, 152, 96, 56)
    );

    RectF imageRect(cardRect.X + 18.0f, cardRect.Y + 16.0f, cardRect.Width - 36.0f, cardRect.Height - 32.0f);
    if (image != nullptr && image->GetLastStatus() == Ok) {
        drawImageFitCropped(g, image, imageRect);
    } else {
        drawLoadFailText(g, L"猫咪图片加载失败", imageRect);
    }
}

static void drawCatSelectPage(Graphics& g, const RECT& clientRect) {
    drawBackgroundCover(g, gLivingRoomBackground, clientRect);
    RectF questionRect(
        static_cast<REAL>(gQuestionRect.left),
        static_cast<REAL>(gQuestionRect.top),
        static_cast<REAL>(gQuestionRect.right - gQuestionRect.left),
        static_cast<REAL>(gQuestionRect.bottom - gQuestionRect.top)
    );
    drawCenteredText(g, gCatSelectQuestionText, questionRect, 24.0f, Color(255, 255, 246, 214), Color(255, 108, 66, 35));
    drawCatCard(g, gCatRects[0], gCatRagdollImage);
    drawCatCard(g, gCatRects[1], gCatCalicoImage);
    drawCatCard(g, gCatRects[2], gCatBlackImage);
}

static void drawHeart(Graphics& g, REAL x, REAL y, REAL size, Color color) {
    SolidBrush brush(color);
    g.FillEllipse(&brush, x, y, size * 0.55f, size * 0.55f);
    g.FillEllipse(&brush, x + size * 0.45f, y, size * 0.55f, size * 0.55f);
    PointF points[3] = {
        PointF(x + size * 0.12f, y + size * 0.33f),
        PointF(x + size * 0.88f, y + size * 0.33f),
        PointF(x + size * 0.5f, y + size)
    };
    g.FillPolygon(&brush, points, 3);
}

static void drawPaw(Graphics& g, REAL x, REAL y, REAL scale, Color color) {
    SolidBrush brush(color);
    g.FillEllipse(&brush, x + scale * 0.35f, y + scale * 0.45f, scale * 0.7f, scale * 0.55f);
    g.FillEllipse(&brush, x, y + scale * 0.18f, scale * 0.22f, scale * 0.24f);
    g.FillEllipse(&brush, x + scale * 0.24f, y, scale * 0.22f, scale * 0.26f);
    g.FillEllipse(&brush, x + scale * 0.53f, y, scale * 0.22f, scale * 0.26f);
    g.FillEllipse(&brush, x + scale * 0.77f, y + scale * 0.18f, scale * 0.22f, scale * 0.24f);
}

static void drawSetupBackground(Graphics& g, const RECT& clientRect) {
    SolidBrush baseBrush(Color(255, 245, 193, 108));
    g.FillRectangle(&baseBrush, 0, 0, clientRect.right, clientRect.bottom);

    drawHeart(g, 70.0f, 80.0f, 60.0f, Color(60, 255, 240, 232));
    drawHeart(g, 1120.0f, 95.0f, 48.0f, Color(52, 255, 234, 220));
    drawHeart(g, 1030.0f, 520.0f, 72.0f, Color(56, 255, 230, 214));
    drawHeart(g, 160.0f, 585.0f, 56.0f, Color(48, 255, 243, 230));
    drawPaw(g, 102.0f, 220.0f, 54.0f, Color(55, 255, 241, 225));
    drawPaw(g, 1130.0f, 250.0f, 68.0f, Color(52, 255, 236, 216));
    drawPaw(g, 940.0f, 635.0f, 58.0f, Color(48, 255, 241, 225));
    drawPaw(g, 260.0f, 470.0f, 48.0f, Color(46, 255, 234, 214));
}

static void drawCenteredText(Graphics& g, const wchar_t* text, const RectF& rect, REAL size, Color fill, Color outline) {
    FontFamily family(L"Microsoft YaHei UI");
    StringFormat format;
    format.SetAlignment(StringAlignmentCenter);
    format.SetLineAlignment(StringAlignmentCenter);

    GraphicsPath shadowPath;
    shadowPath.AddString(text, -1, &family, FontStyleBold, size,
        RectF(rect.X + 2.0f, rect.Y + 4.0f, rect.Width, rect.Height), &format);
    SolidBrush shadowBrush(Color(100, 92, 52, 26));
    g.FillPath(&shadowBrush, &shadowPath);

    GraphicsPath textPath;
    textPath.AddString(text, -1, &family, FontStyleBold, size, rect, &format);
    Pen outlinePen(outline, 4.0f);
    outlinePen.SetLineJoin(LineJoinRound);
    SolidBrush fillBrush(fill);
    g.DrawPath(&outlinePen, &textPath);
    g.FillPath(&fillBrush, &textPath);
}

static void drawPlainWrappedText(Graphics& g, const wchar_t* text, const RectF& rect, REAL size) {
    HDC hdc = g.GetHDC();
    int oldBkMode = SetBkMode(hdc, TRANSPARENT);
    COLORREF oldColor = SetTextColor(hdc, RGB(35, 35, 35));
    HFONT font = getCachedUiFont(static_cast<int>(size), FW_NORMAL);
    HFONT oldFont = static_cast<HFONT>(SelectObject(hdc, font));
    int savedDc = SaveDC(hdc);
    RECT textRect = {
        static_cast<LONG>(rect.X),
        static_cast<LONG>(rect.Y),
        static_cast<LONG>(rect.GetRight()),
        static_cast<LONG>(rect.GetBottom())
    };
    IntersectClipRect(hdc, textRect.left, textRect.top, textRect.right, textRect.bottom);
    DrawTextW(
        hdc,
        text != nullptr ? text : L"",
        -1,
        &textRect,
        DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX | DT_EDITCONTROL
    );
    RestoreDC(hdc, savedDc);
    SelectObject(hdc, oldFont);
    SetTextColor(hdc, oldColor);
    SetBkMode(hdc, oldBkMode);
    g.ReleaseHDC(hdc);
}

static void drawLoadFailText(Graphics& g, const wchar_t* text, const RectF& rect) {
    FontFamily family(L"Microsoft YaHei UI");
    StringFormat format;
    format.SetAlignment(StringAlignmentCenter);
    format.SetLineAlignment(StringAlignmentCenter);
    GraphicsPath textPath;
    textPath.AddString(text, -1, &family, FontStyleBold, 18.0f, rect, &format);
    Pen outlinePen(Color(255, 255, 246, 235), 4.0f);
    outlinePen.SetLineJoin(LineJoinRound);
    SolidBrush fillBrush(Color(255, 126, 78, 48));
    g.DrawPath(&outlinePen, &textPath);
    g.FillPath(&fillBrush, &textPath);
}

static void drawPromptOptionButton(Graphics& g, const RECT& rect, const wchar_t* text) {
    RectF buttonRect(
        static_cast<REAL>(rect.left),
        static_cast<REAL>(rect.top),
        static_cast<REAL>(rect.right - rect.left),
        static_cast<REAL>(rect.bottom - rect.top)
    );

    SolidBrush fillBrush(Color(238, 255, 240, 214));
    Pen edgePen(Color(220, 180, 136, 98), 2.0f);
    g.FillRectangle(&fillBrush, buttonRect);
    g.DrawRectangle(&edgePen, buttonRect);

    drawPlainWrappedText(g, text, RectF(buttonRect.X + 28.0f, buttonRect.Y + 16.0f, buttonRect.Width - 56.0f, buttonRect.Height - 20.0f), 18.0f);
}

static void drawPromptModal(Graphics& g, const RECT& clientRect) {
    if (!gPromptVisible) {
        return;
    }

    SolidBrush dimBrush(Color(72, 56, 36, 24));
    g.FillRectangle(&dimBrush, 0, 0, clientRect.right, clientRect.bottom);

    RectF boxRect(
        static_cast<REAL>(gPromptBoxRect.left),
        static_cast<REAL>(gPromptBoxRect.top),
        static_cast<REAL>(gPromptBoxRect.right - gPromptBoxRect.left),
        static_cast<REAL>(gPromptBoxRect.bottom - gPromptBoxRect.top)
    );

    if (gPromptBoxImage != nullptr && gPromptBoxImage->GetLastStatus() == Ok) {
        g.DrawImage(gPromptBoxImage, boxRect);
    } else {
        drawRoundedPanel(
            g,
            boxRect,
            Color(240, 252, 234, 214),
            Color(246, 243, 222, 194),
            Color(220, 180, 142, 110)
        );
    }

    RectF questionRect(
        static_cast<REAL>(gPromptQuestionRect.left),
        static_cast<REAL>(gPromptQuestionRect.top),
        static_cast<REAL>(gPromptQuestionRect.right - gPromptQuestionRect.left),
        static_cast<REAL>(gPromptQuestionRect.bottom - gPromptQuestionRect.top)
    );
    drawPlainWrappedText(g, gPromptQuestion.c_str(), questionRect, 27.0f);

    for (int i = 0; i < gPromptOptionCount && i < 4; ++i) {
        drawPromptOptionButton(g, gPromptOptionRects[i], gPromptOptions[i].c_str());
    }
}

static void drawCharacterPortrait(Graphics& g, Image* image) {
    if (image == nullptr || image->GetLastStatus() != Ok) {
        return;
    }

    RectF destRect(
        static_cast<REAL>(gStoryPortraitRect.left),
        static_cast<REAL>(gStoryPortraitRect.top),
        static_cast<REAL>(gStoryPortraitRect.right - gStoryPortraitRect.left),
        static_cast<REAL>(gStoryPortraitRect.bottom - gStoryPortraitRect.top)
    );

    REAL srcW = static_cast<REAL>(image->GetWidth());
    REAL srcH = static_cast<REAL>(image->GetHeight());
    REAL srcX = 0.0f;
    REAL srcY = 0.0f;
    REAL drawSrcW = srcW;
    REAL drawSrcH = srcH;

    if (image == gHumanRagdollMaleImage) {
        srcX = srcW * 0.02f;
        srcY = srcH * 0.01f;
        drawSrcW = srcW * 0.96f;
        drawSrcH = srcH * 0.89f;
    } else if (image == gHumanBlackMaleImage) {
        srcX = srcW * 0.02f;
        srcY = srcH * 0.01f;
        drawSrcW = srcW * 0.96f;
        drawSrcH = srcH * 0.88f;
    }

    REAL scale = (destRect.Width / drawSrcW < destRect.Height / drawSrcH) ? (destRect.Width / drawSrcW) : (destRect.Height / drawSrcH);
    REAL drawW = drawSrcW * scale;
    REAL drawH = drawSrcH * scale;
    RectF drawRect(destRect.X + 20.0f, destRect.GetBottom() - drawH, drawW, drawH);
    g.DrawImage(image, drawRect, srcX, srcY, drawSrcW, drawSrcH, UnitPixel);
}

static void drawStoryDialogue(Graphics& g) {
    drawBottomDialog(g);

    int visibleCount = static_cast<int>(gCurrentDialogueText.size());
    if (gVisibleDialogueChars < visibleCount) {
        visibleCount = gVisibleDialogueChars;
    }
    wstring visibleText = gCurrentDialogueText.substr(0, static_cast<size_t>(visibleCount));
    RectF textRect(
        static_cast<REAL>(gStoryTextRect.left),
        static_cast<REAL>(gStoryTextRect.top),
        static_cast<REAL>(gStoryTextRect.right - gStoryTextRect.left),
        static_cast<REAL>(gStoryTextRect.bottom - gStoryTextRect.top)
    );
    Region oldClip;
    g.GetClip(&oldClip);
    g.SetClip(textRect);
    drawPlainWrappedText(g, visibleText.c_str(), textRect, 22.0f);
    g.SetClip(&oldClip, CombineModeReplace);
}

static void drawStoryPage(Graphics& g, const RECT& clientRect) {
    drawBackgroundCover(g, gLivingRoomBackground, clientRect);
    const StoryNode& node = gCurrentStoryNodes[gCurrentStoryNodeIndex];
    drawCharacterPortrait(g, node.useHumanPortrait ? getCurrentHumanImage() : getCurrentCatImage());
    drawStoryDialogue(g);
}

static void drawChoiceButton(Graphics& g, const RECT& rect, const wchar_t* text, bool selected) {
    RectF buttonRect(
        static_cast<REAL>(rect.left),
        static_cast<REAL>(rect.top),
        static_cast<REAL>(rect.right - rect.left),
        static_cast<REAL>(rect.bottom - rect.top)
    );
    SolidBrush fillBrush(selected ? Color(240, 255, 226, 168) : Color(226, 255, 239, 212));
    Pen edgePen(selected ? Color(235, 156, 85, 42) : Color(220, 152, 96, 56), 2.0f);
    g.FillRectangle(&fillBrush, buttonRect);
    g.DrawRectangle(&edgePen, buttonRect);
    drawSimpleCenterText(g, text, buttonRect, 22.0f, Color(255, 108, 66, 35));
}

static void drawNameInput(Graphics& g) {
    Pen linePen(Color(220, 132, 82, 54), 4.0f);
    linePen.SetStartCap(LineCapRound);
    linePen.SetEndCap(LineCapRound);
    g.DrawLine(&linePen,
        static_cast<REAL>(gNameInputRect.left),
        static_cast<REAL>(gNameInputRect.bottom - 2),
        static_cast<REAL>(gNameInputRect.right),
        static_cast<REAL>(gNameInputRect.bottom - 2));
}

static void drawProfileSetupPage(Graphics& g, const RECT& clientRect) {
    drawBackgroundCover(g, gLivingRoomBackground, clientRect);

    RectF genderRect(
        static_cast<REAL>(gGenderQuestionRect.left),
        static_cast<REAL>(gGenderQuestionRect.top),
        static_cast<REAL>(gGenderQuestionRect.right - gGenderQuestionRect.left),
        static_cast<REAL>(gGenderQuestionRect.bottom - gGenderQuestionRect.top)
    );
    drawSimpleCenterText(g, L"要攻略的角色性别是？", genderRect, 28.0f, Color(255, 35, 35, 35));

    drawChoiceButton(g, gGenderButtons[0], L"男", gSelectedGenderChoice == 1);
    drawChoiceButton(g, gGenderButtons[1], L"女", gSelectedGenderChoice == 2);

    RectF namePromptRect(
        static_cast<REAL>(gNamePromptRect.left),
        static_cast<REAL>(gNamePromptRect.top),
        static_cast<REAL>(gNamePromptRect.right - gNamePromptRect.left),
        static_cast<REAL>(gNamePromptRect.bottom - gNamePromptRect.top)
    );
    drawSimpleCenterText(g, L"给猫咪起个名字吧，接下来就由它陪伴你啦", namePromptRect, 22.0f, Color(255, 35, 35, 35));

    drawNameInput(g);
    drawChoiceButton(g, gReadyButtonRect, L"我准备好了", false);
}

static void drawEndingPage(Graphics& g, const RECT& clientRect) {
    drawBackgroundCover(g, gLivingRoomBackground, clientRect);

    RectF titleRect(
        static_cast<REAL>(gEndingTitleRect.left),
        static_cast<REAL>(gEndingTitleRect.top),
        static_cast<REAL>(gEndingTitleRect.right - gEndingTitleRect.left),
        static_cast<REAL>(gEndingTitleRect.bottom - gEndingTitleRect.top)
    );
    drawSimpleCenterText(g, L"这个故事暂时告一段落", titleRect, 30.0f, Color(255, 35, 35, 35));

    drawChoiceButton(g, gEndingButtons[0], L"回到首页", false);
    drawChoiceButton(g, gEndingButtons[1], L"重新开始", false);
    drawChoiceButton(g, gEndingButtons[2], L"结束游戏", false);
}

static void invalidateStoryDialogueArea(HWND hwnd) {
    RECT dirty = gBottomDialogRect;
    dirty.left -= 8;
    dirty.top -= 8;
    dirty.right += 8;
    dirty.bottom += 8;
    InvalidateRect(hwnd, &dirty, FALSE);
}

static void paintScene(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT clientRect = {};
    GetClientRect(hwnd, &clientRect);

    HDC memDc = CreateCompatibleDC(hdc);
    HBITMAP backBuffer = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
    HBITMAP oldBitmap = static_cast<HBITMAP>(SelectObject(memDc, backBuffer));

    Graphics graphics(memDc);
    graphics.SetSmoothingMode(SmoothingModeHighSpeed);
    graphics.SetInterpolationMode(InterpolationModeBilinear);
    graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

    if (gCurrentPage == UiPageStart) {
        updateStartLayout(clientRect);
        if (gBackground != nullptr && gBackground->GetLastStatus() == Ok) {
            REAL srcW = static_cast<REAL>(gBackground->GetWidth());
            REAL srcH = static_cast<REAL>(gBackground->GetHeight());
            REAL cropRight = srcW * 0.065f;
            REAL cropBottom = srcH * 0.08f;
            graphics.DrawImage(
                gBackground,
                RectF(0.0f, 0.0f, static_cast<REAL>(clientRect.right), static_cast<REAL>(clientRect.bottom)),
                0.0f,
                0.0f,
                srcW - cropRight,
                srcH - cropBottom,
                UnitPixel
            );
        } else {
            LinearGradientBrush fallbackBrush(
                Point(0, 0),
                Point(clientRect.right, clientRect.bottom),
                Color(255, 253, 236, 206),
                Color(255, 240, 204, 145)
            );
            graphics.FillRectangle(&fallbackBrush, 0, 0, clientRect.right, clientRect.bottom);
        }

        if (gShowStartButton) {
            drawStartButton(graphics);
        }
    } else if (gCurrentPage == UiPageCatSelect) {
        updateCatSelectLayout(clientRect);
        drawCatSelectPage(graphics, clientRect);
    } else if (gCurrentPage == UiPageProfileSetup) {
        updateProfileSetupLayout(clientRect);
        drawProfileSetupPage(graphics, clientRect);
    } else if (gCurrentPage == UiPageStory && gCurrentStoryNodes != nullptr && gCurrentStoryNodeIndex >= 0) {
        updateStoryLayout(clientRect);
        drawStoryPage(graphics, clientRect);
    } else if (gCurrentPage == UiPageEnding) {
        updateEndingLayout(clientRect);
        drawEndingPage(graphics, clientRect);
    }

    if (gPromptVisible) {
        updatePromptLayout(clientRect);
        drawPromptModal(graphics, clientRect);
    }

    if (gCurrentPage == UiPageStory) {
        drawTopRightHomeButton(graphics);
    }

    BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, memDc, 0, 0, SRCCOPY);
    SelectObject(memDc, oldBitmap);
    DeleteObject(backBuffer);
    DeleteDC(memDc);
    EndPaint(hwnd, &ps);
}

static void syncPageControls(HWND hwnd) {
    if (gNameEdit == nullptr) {
        return;
    }

    RECT clientRect = {};
    GetClientRect(hwnd, &clientRect);
    updateProfileSetupLayout(clientRect);

    if (gCurrentPage == UiPageProfileSetup) {
        int inputHeight = gNameInputRect.bottom - gNameInputRect.top - 10;
        MoveWindow(
            gNameEdit,
            gNameInputRect.left + 8,
            gNameInputRect.top + 2,
            (gNameInputRect.right - gNameInputRect.left) - 16,
            inputHeight,
            TRUE
        );
        ShowWindow(gNameEdit, SW_SHOW);
    } else {
        ShowWindow(gNameEdit, SW_HIDE);
    }
}

static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_ERASEBKGND:
        return 1;
    case WM_SIZE:
        syncPageControls(hwnd);
        return 0;
    case WM_TIMER:
        if (wParam == 1 && gCurrentPage == UiPageStory) {
            if (!gDialogueFullyShown) {
                gVisibleDialogueChars += 2;
                if (gVisibleDialogueChars >= static_cast<int>(gCurrentDialogueText.size())) {
                    gVisibleDialogueChars = static_cast<int>(gCurrentDialogueText.size());
                    gDialogueFullyShown = true;
                    KillTimer(hwnd, 1);
                }
                invalidateStoryDialogueArea(hwnd);
            }
        } else if (wParam == 2 && gCurrentPage == UiPageStart) {
            gShowStartButton = true;
            KillTimer(hwnd, 2);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    case WM_CTLCOLOREDIT: {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdc, RGB(112, 68, 38));
        SetBkColor(hdc, RGB(245, 193, 108));
        return reinterpret_cast<INT_PTR>(gEditBrush);
    }
    case WM_LBUTTONUP: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        if (gCurrentPage == UiPageStory && pointInRect(x, y, gTopRightButtonRect)) {
            returnToPreviousChoice(hwnd);
            return 0;
        }
        if (gPromptVisible && gPromptOptionCount > 0) {
            for (int i = 0; i < gPromptOptionCount && i < 4; ++i) {
                if (pointInRect(x, y, gPromptOptionRects[i])) {
                    gPromptSelectedIndex = i;
                    handlePromptSelection(hwnd, i);
                    InvalidateRect(hwnd, nullptr, TRUE);
                    return 0;
                }
            }
            return 0;
        }
        if (gCurrentPage == UiPageStart) {
            if (gShowStartButton && pointInRect(x, y, gStartButtonRect)) {
                gCurrentPage = UiPageCatSelect;
                syncPageControls(hwnd);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        } else if (gCurrentPage == UiPageCatSelect) {
            if (pointInRect(x, y, gCatRects[0])) {
                gSelectedBreedChoice = 1;
            } else if (pointInRect(x, y, gCatRects[1])) {
                gSelectedBreedChoice = 2;
            } else if (pointInRect(x, y, gCatRects[2])) {
                gSelectedBreedChoice = 4;
            }

            if (gSelectedBreedChoice != 0) {
                closePromptModal();
                gCurrentPage = UiPageProfileSetup;
                syncPageControls(hwnd);
                SetFocus(gNameEdit);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        } else if (gCurrentPage == UiPageProfileSetup) {
            if (pointInRect(x, y, gGenderButtons[0])) {
                gSelectedGenderChoice = 1;
                InvalidateRect(hwnd, nullptr, FALSE);
            } else if (pointInRect(x, y, gGenderButtons[1])) {
                gSelectedGenderChoice = 2;
                InvalidateRect(hwnd, nullptr, FALSE);
            } else if (pointInRect(x, y, gReadyButtonRect)) {
                wchar_t buffer[128] = {};
                GetWindowTextW(gNameEdit, buffer, 128);
                gSelectedCatName = trimWide(buffer);
                if (gSelectedCatName.empty()) {
                    gSelectedCatName = L"奶油";
                }
                if (gSelectedGenderChoice == 0) {
                    gSelectedGenderChoice = 1;
                }
                startStory(hwnd);
            }
        } else if (gCurrentPage == UiPageStory) {
            if (!gDialogueFullyShown) {
                gVisibleDialogueChars = static_cast<int>(gCurrentDialogueText.size());
                gDialogueFullyShown = true;
                KillTimer(hwnd, 1);
                invalidateStoryDialogueArea(hwnd);
            } else {
                advanceStory(hwnd);
            }
        } else if (gCurrentPage == UiPageEnding) {
            if (pointInRect(x, y, gEndingButtons[0])) {
                resetToStart(hwnd);
            } else if (pointInRect(x, y, gEndingButtons[1])) {
                restartGameFlow(hwnd);
            } else if (pointInRect(x, y, gEndingButtons[2])) {
                DestroyWindow(hwnd);
            }
        }
        return 0;
    }
    case WM_PAINT:
        paintScene(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

static bool showStartWindow(HINSTANCE instance) {
    GdiplusStartupInput gdiplusInput;
    if (GdiplusStartup(&gGdiToken, &gdiplusInput, nullptr) != Ok) {
        return true;
    }

    gCurrentPage = UiPageStart;
    gShowStartButton = false;
    gSelectedBreedChoice = 0;
    gSelectedGenderChoice = 0;
    gSelectedCatName.clear();
    closePromptModal();

    wstring backgroundPath = findAssetPath(L"start_menu_new.png");
    if (backgroundPath.empty()) {
        backgroundPath = findAssetPath(L"start_menu.png");
    }
    if (!backgroundPath.empty()) {
        gBackground = Image::FromFile(backgroundPath.c_str(), FALSE);
    }
    wstring livingRoomPath = findAssetPath(L"living_room_bg.png");
    if (!livingRoomPath.empty()) {
        gLivingRoomBackground = Image::FromFile(livingRoomPath.c_str(), FALSE);
    }
    wstring dialogBoxPath = findAssetPath(L"dialog_box.png");
    if (!dialogBoxPath.empty()) {
        gDialogBoxImage = Image::FromFile(dialogBoxPath.c_str(), FALSE);
    }
    wstring promptBoxPath = findAssetPath(L"prompt_box.png");
    if (!promptBoxPath.empty()) {
        gPromptBoxImage = Image::FromFile(promptBoxPath.c_str(), FALSE);
    }
    wstring humanBlackMalePath = findAssetPath(L"human_black_male.png");
    wstring humanBlackFemalePath = findAssetPath(L"human_black_female.png");
    wstring humanCalicoMalePath = findAssetPath(L"human_calico_male.png");
    wstring humanCalicoFemalePath = findAssetPath(L"human_calico_female.png");
    wstring humanRagdollMalePath = findAssetPath(L"human_ragdoll_male.png");
    wstring humanRagdollFemalePath = findAssetPath(L"human_ragdoll_female.png");
    if (!humanBlackMalePath.empty()) {
        gHumanBlackMaleImage = Image::FromFile(humanBlackMalePath.c_str(), FALSE);
    }
    if (!humanBlackFemalePath.empty()) {
        gHumanBlackFemaleImage = Image::FromFile(humanBlackFemalePath.c_str(), FALSE);
    }
    if (!humanCalicoMalePath.empty()) {
        gHumanCalicoMaleImage = Image::FromFile(humanCalicoMalePath.c_str(), FALSE);
    }
    if (!humanCalicoFemalePath.empty()) {
        gHumanCalicoFemaleImage = Image::FromFile(humanCalicoFemalePath.c_str(), FALSE);
    }
    if (!humanRagdollMalePath.empty()) {
        gHumanRagdollMaleImage = Image::FromFile(humanRagdollMalePath.c_str(), FALSE);
    }
    if (!humanRagdollFemalePath.empty()) {
        gHumanRagdollFemaleImage = Image::FromFile(humanRagdollFemalePath.c_str(), FALSE);
    }
    wstring blackCatPath = findAssetPath(L"cat_black.png");
    wstring calicoCatPath = findAssetPath(L"cat_calico.png");
    wstring ragdollCatPath = findAssetPath(L"cat_ragdoll.png");
    if (!blackCatPath.empty()) {
        gCatBlackImage = Image::FromFile(blackCatPath.c_str(), FALSE);
    }
    if (!calicoCatPath.empty()) {
        gCatCalicoImage = Image::FromFile(calicoCatPath.c_str(), FALSE);
    }
    if (!ragdollCatPath.empty()) {
        gCatRagdollImage = Image::FromFile(ragdollCatPath.c_str(), FALSE);
    }

    WNDCLASSW wc = {};
    wc.lpfnWndProc = windowProc;
    wc.hInstance = instance;
    wc.lpszClassName = kWindowClass;
    wc.hCursor = LoadCursor(nullptr, IDC_HAND);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0,
        kWindowClass,
        kWindowTitle,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1280,
        760,
        nullptr,
        nullptr,
        instance,
        nullptr
    );

    if (hwnd == nullptr) {
        if (gBackground != nullptr) {
            delete gBackground;
            gBackground = nullptr;
        }
        if (gLivingRoomBackground != nullptr) {
            delete gLivingRoomBackground;
            gLivingRoomBackground = nullptr;
        }
        if (gDialogBoxImage != nullptr) {
            delete gDialogBoxImage;
            gDialogBoxImage = nullptr;
        }
        if (gPromptBoxImage != nullptr) {
            delete gPromptBoxImage;
            gPromptBoxImage = nullptr;
        }
        if (gHumanBlackMaleImage != nullptr) {
            delete gHumanBlackMaleImage;
            gHumanBlackMaleImage = nullptr;
        }
        if (gHumanBlackFemaleImage != nullptr) {
            delete gHumanBlackFemaleImage;
            gHumanBlackFemaleImage = nullptr;
        }
        if (gHumanCalicoMaleImage != nullptr) {
            delete gHumanCalicoMaleImage;
            gHumanCalicoMaleImage = nullptr;
        }
        if (gHumanCalicoFemaleImage != nullptr) {
            delete gHumanCalicoFemaleImage;
            gHumanCalicoFemaleImage = nullptr;
        }
        if (gHumanRagdollMaleImage != nullptr) {
            delete gHumanRagdollMaleImage;
            gHumanRagdollMaleImage = nullptr;
        }
        if (gHumanRagdollFemaleImage != nullptr) {
            delete gHumanRagdollFemaleImage;
            gHumanRagdollFemaleImage = nullptr;
        }
        if (gCatBlackImage != nullptr) {
            delete gCatBlackImage;
            gCatBlackImage = nullptr;
        }
        if (gCatCalicoImage != nullptr) {
            delete gCatCalicoImage;
            gCatCalicoImage = nullptr;
        }
        if (gCatRagdollImage != nullptr) {
            delete gCatRagdollImage;
            gCatRagdollImage = nullptr;
        }
        GdiplusShutdown(gGdiToken);
        gGdiToken = 0;
        return true;
    }

    gEditBrush = CreateSolidBrush(RGB(245, 193, 108));
    gUiFont = CreateFontW(
        28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        VARIABLE_PITCH, L"Microsoft YaHei UI"
    );
    gNameEdit = CreateWindowExW(
        0,
        L"EDIT",
        L"",
        WS_CHILD | ES_AUTOHSCROLL,
        0, 0, 0, 0,
        hwnd,
        nullptr,
        instance,
        nullptr
    );
    if (gNameEdit != nullptr) {
        SendMessageW(gNameEdit, WM_SETFONT, reinterpret_cast<WPARAM>(gUiFont), TRUE);
        SetWindowTextW(gNameEdit, L"");
        ShowWindow(gNameEdit, SW_HIDE);
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    syncPageControls(hwnd);
    SetTimer(hwnd, 2, 1000, nullptr);

    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (gBackground != nullptr) {
        delete gBackground;
        gBackground = nullptr;
    }
    if (gLivingRoomBackground != nullptr) {
        delete gLivingRoomBackground;
        gLivingRoomBackground = nullptr;
    }
    if (gDialogBoxImage != nullptr) {
        delete gDialogBoxImage;
        gDialogBoxImage = nullptr;
    }
    if (gPromptBoxImage != nullptr) {
        delete gPromptBoxImage;
        gPromptBoxImage = nullptr;
    }
    if (gHumanBlackMaleImage != nullptr) {
        delete gHumanBlackMaleImage;
        gHumanBlackMaleImage = nullptr;
    }
    if (gHumanBlackFemaleImage != nullptr) {
        delete gHumanBlackFemaleImage;
        gHumanBlackFemaleImage = nullptr;
    }
    if (gHumanCalicoMaleImage != nullptr) {
        delete gHumanCalicoMaleImage;
        gHumanCalicoMaleImage = nullptr;
    }
    if (gHumanCalicoFemaleImage != nullptr) {
        delete gHumanCalicoFemaleImage;
        gHumanCalicoFemaleImage = nullptr;
    }
    if (gHumanRagdollMaleImage != nullptr) {
        delete gHumanRagdollMaleImage;
        gHumanRagdollMaleImage = nullptr;
    }
    if (gHumanRagdollFemaleImage != nullptr) {
        delete gHumanRagdollFemaleImage;
        gHumanRagdollFemaleImage = nullptr;
    }
    if (gCatBlackImage != nullptr) {
        delete gCatBlackImage;
        gCatBlackImage = nullptr;
    }
    if (gCatCalicoImage != nullptr) {
        delete gCatCalicoImage;
        gCatCalicoImage = nullptr;
    }
    if (gCatRagdollImage != nullptr) {
        delete gCatRagdollImage;
        gCatRagdollImage = nullptr;
    }
    if (gNameEdit != nullptr) {
        DestroyWindow(gNameEdit);
        gNameEdit = nullptr;
    }
    if (gUiFont != nullptr) {
        DeleteObject(gUiFont);
        gUiFont = nullptr;
    }
    if (gEditBrush != nullptr) {
        DeleteObject(gEditBrush);
        gEditBrush = nullptr;
    }
    GdiplusShutdown(gGdiToken);
    gGdiToken = 0;
    return gStartRequested;
}

int main() {
    HWND consoleWindow = GetConsoleWindow();
    if (consoleWindow != nullptr) {
        ShowWindow(consoleWindow, SW_HIDE);
    }

    HINSTANCE instance = GetModuleHandleW(nullptr);
    showStartWindow(instance);
    return 0;
}

