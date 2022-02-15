

find_program(NPM_COMMAND npm)


if (CMAKE_BUILD_TYPE MATCHES Debug)
   set(BUILD_REACT "echo")
   set(BUILD_REACT_ARGS Skipping react debug build)
else()
    set(BUILD_REACT npm)
    set(BUILD_REACT_ARGS run build)
endif()

set (BUILD_DIRECTORY ${PROJECT_SOURCE_DIR}/build/react/build)


add_custom_command(
    OUTPUT ${BUILD_DIRECTORY}/index.html
    COMMAND ${BUILD_REACT}
    ARGS  ${BUILD_REACT_ARGS}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/react

    DEPENDS 
        src/NumericInput.tsx
        src/RenameDialog.tsx
        src/ToobInputStageView.tsx
        src/ToobCabSimView.tsx
        src/SearchFilter.tsx
        src/Draggable.tsx
        src/SelectHoverBackground.tsx
        src/JackStatusView.tsx
        src/SplitControlView.tsx
        src/MidiBindingsDialog.tsx
        src/WifiChannel.tsx
        src/PluginControlView.tsx
        src/TemporaryDrawer.tsx
        src/logo.svg
        src/AlsaDeviceInfo.tsx
        src/JackServerSettingsDialog.tsx
        src/ZoomedUiControl.tsx
        src/PedalBoardView.tsx
        src/MainPage.tsx
        src/App.tsx
        src/AppThemed.tsx
        src/PresetDialog.tsx
        src/PluginPreset.tsx
        src/react-app-env.d.ts
        src/ToobSpectrumResponseView.tsx
        src/SettingsDialog.tsx
        src/SelectChannelsDialog.tsx
        src/StringBuilder.tsx
        src/ToobPowerStage2View.tsx
        src/SearchControl.tsx
        src/XxxSnippet.tsx
        src/Units.tsx
        src/Jack.tsx
        src/PluginControl.tsx
        src/ResizeResponsiveComponent.tsx
        src/Lv2Plugin.tsx
        src/index.tsx
        src/NoChangePassword.tsx
        src/VuMeter.tsx
        src/ControlViewFactory.tsx
        src/JackServerSettings.tsx
        src/MidiBinding.tsx
        src/UploadDialog.tsx
        src/PiPedalSocket.tsx
        src/PedalBoard.tsx
        src/JackHostStatus.tsx
        src/ZoomedDial.tsx
        src/Rect.tsx
        src/SvgPathBuilder.tsx
        src/IControlViewFactory.tsx
        src/PiPedalModel.tsx
        src/PluginInfoDialog.tsx
        src/reportWebVitals.ts
        src/PiPedalError.tsx
        src/BankDialog.tsx
        src/PluginIcon.tsx
        src/WifiConfigDialog.tsx
        src/PresetSelector.tsx
        src/LoadPluginDialog.tsx
        src/ToobFrequencyResponseView.tsx
        src/WifiConfigSettings.tsx
        src/setupTests.ts
        src/AboutDialog.tsx
        src/index.css
        src/Banks.tsx
        src/DialogEx.tsx
        src/ToobWaveShapeView.tsx
        src/FullScreenIME.tsx
        src/AppThemed.css
        src/PluginPresetSelector.tsx
        src/Utility.tsx
        src/SelectMidiChannelsDialog.tsx
        src/App.test.tsx
        src/MidiBindingView.tsx
        src/SplitUiControls.tsx
        src/PluginClass.tsx
        src/ObservableProperty.tsx
        src/Pedal.tsx
        src/GxTunerControl.tsx
        src/DraggableGrid.tsx
        src/ToobToneStackView.tsx
        src/ToobSpectrumAnalyzerView.tsx
    
        public/css/roboto.css
        public/fonts/Roboto-BlackItalic.woff2
        public/fonts/Roboto-Black.woff2
        public/fonts/Roboto-BoldItalic.woff2
        public/fonts/Roboto-Bold.woff2
        public/fonts/Roboto-Italic.woff2
        public/fonts/Roboto-LightItalic.woff2
        public/fonts/Roboto-Light.woff2
        public/fonts/Roboto-MediumItalic.woff2
        public/fonts/Roboto-Medium.woff2
        public/fonts/Roboto-Regular.woff2
        public/fonts/Roboto-ThinItalic.woff2
        public/fonts/Roboto-Thin.woff2

        public/iso_codes.json
        public/logo512.png
        public/index.html
        public/manifest.json
        public/var
        public/var/current_pedalboard.json
        public/var/uiplugins.json
        public/var/config.json
        public/var/plugin_classes.json
        public/logo192.png
        public/serve.json
        public/robots.txt
        public/sample_lv2_plugins.json
        public/logo192x.png
        public/img/fx_analyzer.svg
        public/img/fx_flanger.svg
        public/img/cx_stereo.svg
        public/img/fx_split_a.svg
        public/img/fx_dial.svg
        public/img/drive_file_rename_outline_black_24dp.svg
        public/img/fx_filter.svg
        public/img/fx_filter_hp.svg
        public/img/cx_mono.svg
        public/img/fx_limiter.svg
        public/img/fx_converter.svg
        public/img/fx_simulator.svg
        public/img/fx_delay.svg
        public/img/fx_reverb.svg
        public/img/fx_empty.svg
        public/img/fx_eq.svg
        public/img/edit_banks.svg
        public/img/fx_gate.svg
        public/img/settings_black_24dp.svg
        public/img/help_outline_black_24dp.svg
        public/img/fx_oscillator.svg
        public/img/fx_split_b.svg
        public/img/fx_phaser.svg
        public/img/fx_amplifier.svg
        public/img/fx_plugin.svg
        public/img/ic_drawer.svg
        public/img/ic_drawer_2.png
        
        public/img/fx_terminal.svg
        public/img/fx_spatial.svg
        public/img/fx_pitch.svg
        public/img/ic_logo.svg
        public/img/fx_flanger2.svg
        public/img/fx_function.svg
        public/img/fx_lr.svg
        public/img/fx_generator.svg
        public/img/fx_spectral.svg
        public/img/fx_chorus.svg
        public/img/fx_utility.svg
        public/img/fx_mixer.svg
        public/img/ic_presets.svg
        public/img/fx_instrument.svg
        public/img/fx_modulator.svg
        public/img/fx_compressor.svg
        public/img/fx_constant.svg
        public/img/fx_distortion.svg
        public/img/save_bank_as.svg
        public/favicon.ico

)

add_custom_target (
    ReactBuild ALL
    DEPENDS ${BUILD_DIRECTORY}/index.html
)
