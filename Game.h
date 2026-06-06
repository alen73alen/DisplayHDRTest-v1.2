//********************************************************* 
// 
// Copyright (c) Microsoft. All rights reserved. 
// This code is licensed under the MIT License (MIT). 
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY 
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR 
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT. 
// 
//*********************************************************

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "Basicmath.h"
#include <map>

#include <winrt\Windows.Devices.Display.h>
#include <winrt\Windows.Devices.Display.Core.h>
#include <winrt\Windows.Devices.Enumeration.h>

/**
 * @brief Structure to hold raw output description from the display.
 */
struct rawOutputDesc
{
	float MaxLuminance;             ///< Maximum luminance in nits.
	float MaxFullFrameLuminance;    ///< Maximum full-frame average luminance in nits.
	float MinLuminance;             ///< Minimum luminance in nits.
};

/**
 * @brief Main game class that implements a D3D11 device and provides a game loop for HDR testing.
 *
 * This class handles the initialization of Direct3D resources, manages different HDR test patterns,
 * and processes display metadata for high dynamic range output.
 */
class Game : public DX::IDeviceNotify
{
private:
    /**
     * @brief Supported VESA DisplayHDR tiers.
     */
    enum TestingTier
    {
        DisplayHDR400   = 0,
		DisplayHDR500   = 1,
		DisplayHDR600   = 2,
        DisplayHDR1000  = 3,
        DisplayHDR1400  = 4,
		DisplayHDR2000  = 5,
		DisplayHDR3000  = 6,
        DisplayHDR4000  = 7,
        DisplayHDR6000  = 8,
        DisplayHDR10000 = 9
    };

    /**
     * @brief Supported color gamuts for metadata signaling.
     */
    enum ColorGamut
    {
        GAMUT_Native = 0,
        GAMUT_sRGB   = 1,
        GAMUT_Adobe  = 2,
        GAMUT_DCIP3  = 3,
        GAMUT_BT2100 = 4,
        GAMUT_ACES   = 5,
    };

    #define NUM_WBRACKETS 8
    UINT32 WhiteLevelBrackets[NUM_WBRACKETS] = { 50, 100, 200, 250, 300, 500, 700, 1000 };

    /**
     * @brief Resources required by a specific test pattern.
     */
    struct TestPatternResources
    {
        std::wstring testTitle; ///< Title displayed on screen.
        std::wstring imageFilename; ///< Path to the image file, if any.
        std::wstring effectShaderFilename; ///< Path to the compiled shader file, if any.
        GUID effectClsid; ///< CLSID of the D2D effect.

        Microsoft::WRL::ComPtr<IWICBitmapSource> wicSource; ///< WIC bitmap source for images.
        Microsoft::WRL::ComPtr<ID2D1ImageSourceFromWic> d2dSource; ///< D2D image source.
        Microsoft::WRL::ComPtr<ID2D1Effect> d2dEffect; ///< D2D effect instance.
        bool imageIsValid; ///< Flag indicating if the image resource is loaded correctly.
        bool effectIsValid; ///< Flag indicating if the effect resource is loaded correctly.
    };

public:

    /**
     * @brief Construct a new Game object.
     * @param appTitle The title of the application window.
     */
    Game(PWSTR appTitle);

    /**
     * @brief Supported checkerboard patterns for contrast tests.
     */
    enum class Checkerboard
    {
        Cb6x4,      ///< 6x4 grid.
        Cb4x3,      ///< 4x3 grid.
        Cb4x3not,   ///< Inverse 4x3 grid.
    };

    /**
     * @brief Enumeration of all available test patterns.
     */
	enum class TestPattern
	{
		StartOfTest, ///< Welcome screen.
		ConnectionProperties, ///< Display connection details.
		PanelCharacteristics, ///< Reported display capabilities.
		ResetInstructions, ///< Instructions for OSD reset.
        PQLevelsInNits, ///< PQ EOTF reference patches.
        WarmUp, ///< Constant luminance screen for thermal stabilization.
        TenPercentPeak,             ///< 1. 10% area peak luminance.
        TenPercentPeakMAX,          ///< 1. MAX 10% area peak luminance at 10k nits.
        FlashTest,                  ///< 2. Brief peak luminance flash.
        FlashTestMAX,               ///< 2. MAX Brief peak luminance flash at 10k nits.
        LongDurationWhite,          ///< 3. Full-frame white sustained.
        FullFramePeak,              ///< 3. MAX Full-frame white at 10k nits.
		DualCornerBox,				///< 4. Total contrast test for TrueBlack.
		StaticContrastRatio,		///< 5. Static contrast checkerboard.
		ActiveDimming,				///< 5.1 Active dimming test at 50 nits.
		ActiveDimmingDark,			///< 5.2 Active dimming test at dark levels.
		ActiveDimmingSplit,			///< 5.3 Active dimming split-screen test.
		ColorPatches,               ///< 6. Native primary color patches (10% OPR).
		ColorPatchesFull,           ///< 6. Native primary color patches (Fullscreen).
		BitDepthPrecision,          ///< 7. Gradient test for bit-depth quantification.
        RiseFallTime,               ///< 8. Response time test (Black-to-Peak).
		ProfileCurve,               ///< 9. PQ curve tracking validation.
        LocalDimmingContrast,       ///< v1.2.1 Dimming contrast.
        BlackLevelHDRvsSDR,         ///< v1.2.2 Black level comparison.
        BlackLevelCrush,            ///< v1.2.3 Black level visibility test.
        SubTitleFlicker,            ///< v1.2.4 Flicker test with simulated subtitles.
        XRiteColors,			    ///< v1.2.5 Standard color patch sequence.
		EndOfMandatoryTests,        ///< Marker for completion of VESA mandatory set.
        SharpeningFilter,           ///< Informative sharpening test.
		ToneMapSpike,				///< Informative tone mapping test.
        TextQuality,                ///< ClearType and grayscale text quality.
        OnePixelLinesBW,            ///< Single-pixel black/white lines.
        OnePixelLinesRG,            ///< Single-pixel red/green lines.
        ColorPatches709,            ///< Rec. 709 primary color patches.
        FullFrameSDRWhite,          ///< SDR white level reference.
        FullFrameSDRWhiteWithHDR,   ///< SDR background with HDR window.
		CalibrateMaxEffectiveValue,		///< Tone map limit calibration.
		CalibrateMaxEffectiveFullFrameValue,	///< MaxFALL calibration.
		CalibrateMinEffectiveValue,		///< Black crush calibration.
        StaticGradient,             ///< Static linear gradient.
        AnimatedGrayGradient,       ///< Animated grayscale gradient.
        AnimatedColorGradient,      ///< Animated color gradient.
		BlackLevelHdrCorners,       ///< 4. Legacy corner test.
		BlackLevelSdrTunnel,        ///< 5. Legacy tunnel test.
		ColorPatchesMAX,            ///< 6. Legacy MAX color patches.
		EndOfTest, ///< Final screen.
        Cooldown, ///< 60s black screen for cooling.
    };

    /**
     * @brief Initializes the Direct3D resources and associates the game with a window.
     * @param window The handle to the window.
     * @param width The width of the window.
     * @param height The height of the window.
     */
    void Initialize(HWND window, int width, int height);

    /**
     * @brief Executes one frame of the game loop (Update and Render).
     */
    void Tick();

    // IDeviceNotify methods
    virtual void OnDeviceLost() override;
    virtual void OnDeviceRestored() override;

    // Window message handlers
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowSizeChanged(int width, int height);
    void OnDisplayChange();

    /**
     * @brief Gets the default window size.
     */
    void GetDefaultSize(int& width, int& height) const;

    // Test pattern control methods
    void SetTestPattern(TestPattern testPattern);
    void ChangeTestPattern(bool increment);
    void ChangeSubtest( INT32 increment );
    void SetShift( bool shift );
    bool GetShift( void );
    void ToggleXRitePatchAuto( void );
    void ChangeXRitePatchDisplayTime(INT32 increment);
    void SelectWhiteLevel(INT32 incrememnt);
    void StartTestPattern(void);
    void ChangeGradientColor(float deltaR, float deltaG, float deltaB);
    void ChangeBackBufferFormat(DXGI_FORMAT fmt);
    void ChangeCheckerboard( INT32 increment);
    bool PauseAnimation();
    bool ToggleSubtitle();
    bool ToggleInfoTextVisible();

    /**
     * @brief Resets HDR metadata to neutral (panel-defined) values.
     */
    void SetMetadataNeutral();

	void PrintMetadata(   ID2D1DeviceContext2* ctx, bool blackText = false );
    void PrintTestingTier(ID2D1DeviceContext2* ctx, bool blackText = false);

private:

    void ConstructorInternal();

    void Update(DX::StepTimer const& timer);
    void UpdateDxgiColorimetryInfo();
	void InitEffectiveValues();
    void SetMetadata(float max, float avg, ColorGamut gamut);
    void Render();
	bool CheckHDR_On();
    bool CheckForDefaults();
	void DrawLogo(ID2D1DeviceContext2 *ctx, float c );
    void DrawChecker6x4(  ID2D1DeviceContext2* ctx, float colorL, float colorR );
    void DrawChecker4x3(  ID2D1DeviceContext2* ctx, float colorL, float colorR );
    void DrawChecker4x3n( ID2D1DeviceContext2* ctx, float colorL, float colorR );
    TestingTier GetTestingTier();
    WCHAR *GetTierName(TestingTier tier);
	float GetTierLuminance(Game::TestingTier tier);

    // Test pattern generators
    void GenerateTestPattern_StartOfTest(ID2D1DeviceContext2* ctx);
    void GenerateTestPattern_ConnectionProperties(ID2D1DeviceContext2* ctx);
    void GenerateTestPattern_PanelCharacteristics(ID2D1DeviceContext2* ctx);
    void GenerateTestPattern_ResetInstructions(ID2D1DeviceContext2* ctx);
	void GenerateTestPattern_CalibrateMaxEffectiveValue(ID2D1DeviceContext2* ctx);
	void GenerateTestPattern_CalibrateMaxFullFrameValue(ID2D1DeviceContext2* ctx);
	void GenerateTestPattern_CalibrateMinEffectiveValue(ID2D1DeviceContext2* ctx);
	void GenerateTestPattern_PQLevelsInNits(ID2D1DeviceContext2* ctx);
    void GenerateTestPattern_WarmUp(ID2D1DeviceContext2* ctx);
    void GenerateTestPattern_Cooldown(ID2D1DeviceContext2* ctx);
    void GenerateTestPattern_TenPercentPeak(ID2D1DeviceContext2* ctx);
    void GenerateTestPattern_TenPercentPeakMAX(ID2D1DeviceContext2* ctx);
    void GenerateTestPattern_FlashTest(ID2D1DeviceContext2* ctx);
    void GenerateTestPattern_FlashTestMAX(ID2D1DeviceContext2* ctx);
	void GenerateTestPattern_LongDurationWhite(ID2D1DeviceContext2* ctx);
	void GenerateTestPattern_DualCornerBox(ID2D1DeviceContext2* ctx);
	void GenerateTestPattern_StaticContrastRatio(ID2D1DeviceContext2 * ctx);
	void GenerateTestPattern_ActiveDimming(ID2D1DeviceContext2 * ctx);
	void GenerateTestPattern_ActiveDimmingDark(ID2D1DeviceContext2 * ctx);
	void GenerateTestPattern_ActiveDimmingSplit(ID2D1DeviceContext2 * ctx);
    void GenerateTestPattern_ColorPatches(   ID2D1DeviceContext2* ctx, bool full = false );
    void GenerateTestPattern_ColorPatchesMAX(ID2D1DeviceContext2* ctx, float OPR );
    void GenerateTestPattern_BitDepthPrecision(ID2D1DeviceContext2* ctx);
	void GenerateTestPattern_RiseFallTime(ID2D1DeviceContext2* ctx);
	void GenerateTestPattern_ProfileCurve(ID2D1DeviceContext2* ctx);
    void GenerateTestPattern_LocalDimmingContrast(ID2D1DeviceContext2* ctx);
    void GenerateTestPattern_BlackLevelHDRvsSDR(ID2D1DeviceContext2* ctx);
    void GenerateTestPattern_BlackLevelCrush(ID2D1DeviceContext2* ctx);
    void GenerateTestPattern_SubTitleFlicker(ID2D1DeviceContext2* ctx);
	void GenerateTestPattern_XRiteColors(ID2D1DeviceContext2* ctx);
    void GenerateTestPattern_EndOfMandatoryTests(ID2D1DeviceContext2* ctx);
    void GenerateTestPattern_FullFramePeak(ID2D1DeviceContext2* ctx);
	void GenerateTestPattern_BlackLevelHdrCorners(ID2D1DeviceContext2* ctx);
	void GenerateTestPattern_SharpeningFilter(ID2D1DeviceContext2* ctx);
	void GenerateTestPattern_ToneMapSpike(ID2D1DeviceContext2* ctx);
	void GenerateTestPattern_FullFrameSDRWhite(ID2D1DeviceContext2* ctx);
    void GenerateTestPattern_FullFrameSDRWhiteWithHDR(ID2D1DeviceContext2* ctx);
    void GenerateTestPattern_ColorPatches709(ID2D1DeviceContext2 * ctx);
    void GenerateTestPattern_StaticGradient(ID2D1DeviceContext2* ctx);
    void GenerateTestPattern_AnimatedGrayGradient(ID2D1DeviceContext2* ctx);
    void GenerateTestPattern_AnimatedColorGradient(ID2D1DeviceContext2* ctx);
	void GenerateTestPattern_BlackLevelSdrTunnel(ID2D1DeviceContext2* ctx);
	void GenerateTestPattern_BlackLevelSdrTunnelTrueBlack(ID2D1DeviceContext2* ctx);
	void GenerateTestPattern_EndOfTest(ID2D1DeviceContext2* ctx);

    void GenerateTestPattern_ImageCommon(ID2D1DeviceContext2* ctx, TestPatternResources resources);

    void Clear();
    void RenderText(ID2D1DeviceContext2* ctx, IDWriteTextFormat* fmt, std::wstring text, D2D1_RECT_F textPos, bool useBlackText = false);

    void CreateDeviceIndependentResources();
    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();
    void LoadTestPatternResources(TestPatternResources* resources);
    void LoadImageResources(TestPatternResources* resources);
    void LoadEffectResources(TestPatternResources* resources);

    // Device resources.
    std::unique_ptr<DX::DeviceResources>    m_deviceResources;
    DXGI_ADAPTER_DESC                                       m_adapterDesc;

    winrt::hstring                                          m_monitorName;
    winrt::Windows::Devices::Display::DisplayMonitorConnectionKind        m_connectionKind;
    winrt::Windows::Devices::Display::DisplayMonitorPhysicalConnectorKind m_physicalConnectorKind;
    winrt::Windows::Devices::Display::DisplayMonitorDescriptorKind        m_connectionDescriptorKind;

    DXGI_RATIONAL                                           m_verticalSyncRate;
    DWORD                                                   m_displayFrequency;


    Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush>        m_gradientBrush;
    Microsoft::WRL::ComPtr<IDWriteTextLayout>               m_testTitleLayout;
    Microsoft::WRL::ComPtr<IDWriteTextLayout>               m_panelInfoTextLayout;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>            m_whiteBrush;
	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>            m_blackBrush;
	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>            m_redBrush;

    DXGI_OUTPUT_DESC1                                       m_outputDesc;
	rawOutputDesc											m_rawOutDesc;

    // Device independent resources.
    Microsoft::WRL::ComPtr<IDWriteTextFormat>               m_smallFormat;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>               m_largeFormat;
    Microsoft::WRL::ComPtr<IDWriteTextFormat>               m_monospaceFormat;
    D2D1_RECT_F                                             m_testTitleRect;
    D2D1_RECT_F                                             m_largeTextRect;
	D2D1_RECT_F												m_MetadataTextRect;
    D2D1_RECT_F                                             m_TestingTierTextRect;
    TestingTier                                             m_testingTier;
    TestPattern                                             m_currentTest;
    TestPattern                                             m_cachedTest;
    bool                                                    m_shiftKey;
    INT32                                                   m_currentColor;
    INT32                                                   m_currentBlack;
    INT32													m_currentProfileTile;
    INT32                                                   m_modeWidth;
    INT32                                                   m_modeHeight;
    float                                                   m_snoodDiam;

	UINT32													m_maxPQCode;
	INT32													m_maxProfileTile;
    float                                                   m_flashOn;
    Checkerboard                                            m_checkerboard;
    D2D1_COLOR_F                                            m_gradientColor;
    INT32                                                   m_LocalDimmingBars;
    INT32                                                   m_subtitleVisible;
    INT32                                                   m_currentXRiteIndex;
    bool                                                    m_XRitePatchAutoMode;
    float                                                   m_XRitePatchDisplayTime;
    INT32                                                   m_whiteLevelBracket;
    INT32                                                   m_XRiteIntensity;
    float                                                   m_gradientAnimationBase;
    bool                                                    m_bPaused;
    bool                                                    m_showExplanatoryText;
    float                                                   m_testTimeRemainingSec;
    float                                                   m_gamutVolume;

	float													m_maxEffectivesRGBValue;
	float													m_maxFullFramesRGBValue;
	float													m_minEffectivesRGBValue;
	float													m_maxEffectivePQValue;
	float													m_maxFullFramePQValue;
	float													m_minEffectivePQValue;
    float                                                   m_staticContrastPQValue;
    float                                                   m_staticContrastsRGBValue;
    float													m_activeDimming50PQValue;
	float													m_activeDimming05PQValue;
    float													m_activeDimming50sRGBValue;
    float													m_activeDimming05sRGBValue;

	bool                                                    m_newTestSelected;
    bool                                                    m_dxgiColorInfoStale;
	DXGI_HDR_METADATA_HDR10									m_Metadata;
	ColorGamut												m_MetadataGamut;


    std::map<TestPattern, TestPatternResources>             m_testPatternResources;
    std::wstring                                            m_hideTextString;

    DX::StepTimer                           m_timer;
    float                                   m_totalTime;

    PWSTR                                   m_appTitle;
};
