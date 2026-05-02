// Story flow, drawing, layout, rendering

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
    } else if (image == gHumanCalicoFemaleImage) {
        srcX = srcW * 0.01f;
        srcY = srcH * 0.01f;
        drawSrcW = srcW * 0.98f;
        drawSrcH = srcH * 0.93f;
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
    drawSimpleCenterText(g, L"再次睁眼时，窗台边的位置还是空荡荡的，可指尖的温度还未消散。你知道的，它来过。", titleRect, 24.0f, Color(255, 35, 35, 35));

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

