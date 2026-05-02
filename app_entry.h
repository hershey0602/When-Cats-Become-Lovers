// Window procedure, startup, main

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

