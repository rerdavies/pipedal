

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
    src/svg/ic_save_bank_as.svg
    src/svg/fx_constant.svg
    src/svg/fx_distortion.svg
    src/svg/ic_help_outline.svg
    src/svg/fx_oscillator.svg
    src/svg/fx_filter.svg
    src/svg/fx_function.svg
    src/svg/fx_generator.svg
    src/svg/fx_compressor.svg
    src/svg/file_download_black_24dp.svg
    src/svg/fx_filter_hp.svg
    src/svg/fx_plugin.svg
    src/svg/fx_error.svg
    src/svg/fx_gate.svg
    src/svg/ic_midi.svg
    src/svg/ic_pluginpreset.svg
    src/svg/fx_split_a.svg
    src/svg/fx_utility.svg
    src/svg/ic_pluginpreset2.svg
    src/svg/fx_parametric_eq.svg
    src/svg/fx_terminal.svg
    src/svg/ic_presets.svg
    src/svg/fx_flanger.svg
    src/svg/fx_analyzer.svg
    src/svg/fx_delay.svg
    src/svg/fx_spectral.svg
    src/svg/fx_chorus.svg
    src/svg/fx_simulator.svg
    src/svg/drive_file_rename_outline_black_24dp.svg
    src/svg/fx_mixer.svg
    src/svg/fx_eq.svg
    src/svg/fx_modulator.svg
    src/svg/file_upload_black_24dp.svg
    src/svg/fx_pitch.svg
    src/svg/fx_phaser.svg
    src/svg/fx_amplifier.svg
    src/svg/fx_lr.svg
    src/svg/fx_instrument.svg
    src/svg/fx_reverb.svg
    src/svg/ic_edit_banks.svg
    src/svg/fx_limiter.svg
    src/svg/fx_empty.svg
    src/svg/fx_dial.svg
    src/svg/fx_flanger2.svg
    src/svg/fx_split_b.svg
    src/svg/fx_spatial.svg
    src/svg/fx_converter.svg
    src/svg/old_delete_outline_24dp.svg
    src/svg/ic_settings.svg
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
    src/DarkMode.tsx
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
    src/SelectThemeDialog.tsx
    src/PluginInfoDialog.tsx
    src/SearchFilter.tsx
    src/SearchControl.tsx
    src/JackServerSettings.tsx
    src/WifiChannel.tsx
    src/WifiConfigDialog.tsx
    src/FilePropertyDirectoryTree.tsx
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
    src/FilePropertyDirectorySelectDialog.tsx
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
            
)

add_custom_target (
    ReactBuild ALL
    DEPENDS ${BUILD_DIRECTORY}/index.html
)
