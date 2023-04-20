

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
    src/OkCancelDialog.tsx
    src/PluginControl.tsx
    src/reportWebVitals.ts
    src/JackHostStatus.tsx
    src/PluginPresetsDialog.tsx
    src/ToobSpectrumAnalyzerView.tsx
    src/MidiBindingsDialog.tsx
    src/MidiBinding.tsx
    src/TemporaryDrawer.tsx
    src/ToobFrequencyResponseView.tsx
    src/index.css
    src/PluginOutputControl.tsx
    src/BankDialog.tsx
    src/VuMeter.tsx
    src/StringBuilder.tsx
    src/FilePropertyDialog.tsx
    src/GxTunerControl.tsx
    src/App.tsx
    src/GxTunerView.tsx
    src/Rect.tsx
    src/ToobSpectrumResponseView.tsx
    src/DialogEx.tsx
    src/GovernorSettings.tsx
    src/FilePropertyControl.tsx
    src/JackServerSettingsDialog.tsx
    src/ZoomedUiControl.tsx
    src/Pedal.tsx
    src/FullScreenIME.tsx
    src/PluginPreset.tsx
    src/SystemMidiBindingsDialog.tsx
    src/AppThemed.tsx
    src/MainPage.tsx
    src/PiPedalModel.tsx
    src/PedalboardView.tsx
    src/ControlViewFactory.tsx
    src/PiPedalSocket.tsx
    src/PluginClass.tsx
    src/SplitControlView.tsx
    src/logo.svg
    src/ToobToneStackView.tsx
    src/Banks.tsx
    src/ToobPowerStage2View.tsx
    src/ListSelectDialog.tsx
    src/ToobWaveShapeView.tsx
    src/PluginInfoDialog.tsx
    src/SearchFilter.tsx
    src/SearchControl.tsx
    src/JackServerSettings.tsx
    src/WifiChannel.tsx
    src/WifiConfigDialog.tsx
    src/App.test.tsx
    src/PluginPresetSelector.tsx
    src/ObservableProperty.tsx
    src/PresetDialog.tsx
    src/AndroidHost.tsx
    src/SelectHoverBackground.tsx
    src/WifiDirectConfigDialog.tsx
    src/WifiDirectConfigSettings.tsx
    src/WifiConfigSettings.tsx
    src/Draggable.tsx
    src/JsonAtom.tsx
    src/SplitUiControls.tsx
    src/SelectChannelsDialog.tsx
    src/ToobMLView.tsx
    src/IControlViewFactory.tsx
    src/AppThemed.css
    src/ZoomedDial.tsx
    src/Units.tsx
    src/UploadPresetDialog.tsx
    src/NumericInput.tsx
    src/Pedalboard.tsx
    src/SelectMidiChannelsDialog.tsx
    src/AlsaMidiDeviceInfo.tsx
    src/ResizeResponsiveComponent.tsx
    src/RenameDialog.tsx
    src/SystemMidiBindingView.tsx
    src/OldDeleteIcon.tsx
    src/ToobInputStageView.tsx
    src/XxxSnippet.tsx
    src/NoChangePassword.tsx
    src/react-app-env.d.ts
    src/PiPedalError.tsx
    src/AlsaDeviceInfo.tsx
    src/index.tsx
    src/SettingsDialog.tsx
    src/PluginControlView.tsx
    src/PresetSelector.tsx
    src/ToobCabSimView.tsx
    src/Utility.tsx
    src/Lv2Plugin.tsx
    src/Jack.tsx
    src/JackStatusView.tsx
    src/AboutDialog.tsx
    src/setupTests.ts
    src/DraggableGrid.tsx
    src/SvgPathBuilder.tsx
    src/LoadPluginDialog.tsx
    src/MidiBindingView.tsx
    src/PluginIcon.tsx
    src/UploadFileDialog.tsx
                
    public/
    public/css
    public/css/roboto.css
    public/logo192x.png
    public/robots.txt
    public/img
    public/img/ic_drawer_2.png
    public/img/fx_constant.svg
    public/img/cx_mono.svg
    public/img/cx_stereo.svg
    public/img/fx_distortion.svg
    public/img/fx_oscillator.svg
    public/img/settings_black_24dp.svg
    public/img/fx_filter.svg
    public/img/delete_outline_black_24dp.svg
    public/img/fx_function.svg
    public/img/fx_generator.svg
    public/img/old_delete_outline_black_24dp.svg
    public/img/ic_bank.svg
    public/img/fx_compressor.svg
    public/img/file_download_black_24dp.svg
    public/img/fx_filter_hp.svg
    public/img/fx_plugin.svg
    public/img/fx_error.svg
    public/img/fx_gate.svg
    public/img/cc-by.png
    public/img/ic_midi.svg
    public/img/ic_pluginpreset.svg
    public/img/fx_split_a.svg
    public/img/fx_utility.svg
    public/img/ic_navigate_before.svg
    public/img/ic_drawer_2.svg
    public/img/ic_pluginpreset2.svg
    public/img/fx_parametric_eq.svg
    public/img/fx_terminal.svg
    public/img/ic_presets.svg
    public/img/VST_Logo_Steinberg.ico
    public/img/fx_flanger.svg
    public/img/fx_analyzer.svg
    public/img/fx_delay.svg
    public/img/fx_spectral.svg
    public/img/edit_banks.svg
    public/img/fx_chorus.svg
    public/img/fx_simulator.svg
    public/img/drive_file_rename_outline_black_24dp.svg
    public/img/fx_mixer.svg
    public/img/fx_eq.svg
    public/img/fx_modulator.svg
    public/img/file_upload_black_24dp.svg
    public/img/fx_pitch.svg
    public/img/ic_navigate_next.svg
    public/img/fx_phaser.svg
    public/img/ic_drawer.svg
    public/img/fx_amplifier.svg
    public/img/save_bank_as.svg
    public/img/fx_lr.svg
    public/img/fx_instrument.svg
    public/img/fx_reverb.svg
    public/img/fx_limiter.svg
    public/img/VST_Logo_Steinberg.png
    public/img/fx_empty.svg
    public/img/fx_dial.svg
    public/img/ic_logo.svg
    public/img/help_outline_black_24dp.svg
    public/img/fx_flanger2.svg
    public/img/old_delete_outline_white_24dp.svg
    public/img/fx_split_b.svg
    public/img/fx_spatial.svg
    public/img/fx_converter.svg
    public/img/vst.svg
    public/img/vst.png
    public/sample_lv2_plugins.json
    public/serve.json
    public/fonts
    public/fonts/Roboto-LightItalic.woff2
    public/fonts/Roboto-Light.woff2
    public/fonts/Roboto-BlackItalic.woff2
    public/fonts/Roboto-ThinItalic.woff2
    public/fonts/Roboto-Bold.woff2
    public/fonts/Roboto-Italic.woff2
    public/fonts/Roboto-Black.woff2
    public/fonts/Roboto-BoldItalic.woff2
    public/fonts/Roboto-Regular.woff2
    public/fonts/Roboto-Medium.woff2
    public/fonts/Roboto-Thin.woff2
    public/fonts/LICENSE.txt
    public/fonts/Roboto-MediumItalic.woff2
    public/favicon.ico
    public/iso_codes.json
    public/logo192.png
    public/logo512.png
    public/index.html
    public/manifest.json
    public/var
    public/var/config.json
    public/var/current_pedalboard.json
    public/var/uiplugins.json
    public/var/plugin_classes.json
    
)

add_custom_target (
    ReactBuild ALL
    DEPENDS ${BUILD_DIRECTORY}/index.html
)
