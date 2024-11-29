// Copyright (c) 2024 Robin E. R. Davies
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

/*
    Preamble:

    The choice of backend for this project was unfortunate. The ftxui project has ungodly horrible problems
    dealing with mouse and keyboard focus. Really embarassingly horrible problems. And no real sensible
    support for modal dialogs. As well as deficiences in keyboard handling (no way to intercept the ESC key, for example,
    which would have been a nice feature).

    So the code that follows is .... unpleasant, as it does its best to work around severe problesm and
    deficiencies in FTXUI.

    Tempting as it is to rewrite from scratch, FTXUI *probably* deals with unpleasant platform issues and
    complications in ncurses, which may not be entirely straighforward on windows. The tab/nav/cursor implementation
    is also *almost* nice (although being able to use the enter key for OK in dialogs would have been nice, and ESC for
    cancel).

    Things to watch out for:  Focus handling in the  FTXUI RadioBox control was unsalvageable. This code hacks together
    a replacement implementation using Checkboxes instead. Checkbox focus handling was also broken, but is (just barely)
    salageable. Also broken under window resizing. (It recovers when you mose over).

    Nevertheless, FTXUI is truly awful. Do not use this package. You have been warned.
*/

#undef _GLIBCXX_DEBUG

#include "ftxui/component/captured_mouse.hpp"     // for ftxui
#include "ftxui/component/component.hpp"          // for Input, Renderer, Vertical
#include "ftxui/component/component_base.hpp"     // for ComponentBase
#include "ftxui/component/component_options.hpp"  // for InputOption
#include "ftxui/component/screen_interactive.hpp" // for Component, ScreenInteractive
#include "ftxui/dom/elements.hpp"                 // for text, hbox, separator, Element, operator|, vbox, border
#include "ftxui/util/ref.hpp"                     // for Ref
#include "ftxui/component/animation.hpp"
#include <chrono>
#include <thread>
#include <atomic>
#include "BootConfig.hpp"
#include <array>
#include "CommandLineParser.hpp"
#include "PrettyPrinter.hpp"
#include "SysExec.hpp"


using namespace ftxui;
using Clock = ftxui::animation::Clock;
using namespace pipedal;

std::string spinnerText(const Clock::time_point &startTime)
{
    using namespace std::chrono;

    constexpr int MAX_LENGTH = 40;
    constexpr int N_DOTS = 3;

    int64_t ms = std::chrono::duration_cast<milliseconds>(Clock::now() - startTime).count();

    int64_t position = (ms * MAX_LENGTH / 3000) % (MAX_LENGTH + N_DOTS);

    char result[MAX_LENGTH - 1];
    for (int64_t i = 0; i < MAX_LENGTH; ++i)
    {
        if (i >= position && i < position + N_DOTS)
        {
            result[i] = '.';
        }
        else
        {
            result[i] = ' ';
        }
    }
    result[MAX_LENGTH] = '\0';
    return result;
}

RadioboxOption CustomRadioboxOption(int *value, int *forceFocus)
{
    RadioboxOption option = RadioboxOption::Simple();
    if (*forceFocus >= 0)
    {
        // option.focused_entry = *forceFocus;
        *forceFocus = -1;
    }
    option.on_change = [value, forceFocus]() mutable
    {
        *forceFocus = *value;
    };

    option.transform = [](const EntryState &s) -> Element
    {
#if 0
    // Microsoft terminal do not use fonts able to render properly the default
    // radiobox glyph.
    auto prefix = text(s.state ? "(*) " : "( ) ");  // NOLINT
#else
        auto prefix = text(s.state ? "◉ " : "○ "); // NOLINT
#endif
        auto t = text(s.label);
        if (s.active != s.focused)
        {
            s.label;
        }
        if (s.active)
        {
            t = t | color(Color::Red);
        }
        if (s.focused)
        {
            t = t | bgcolor(Color::GrayDark);
        }

        return hbox({prefix, t});
    };
    return option;
}
CheckboxOption CustomCheckboxOption(bool *forceFocus)
{
    CheckboxOption option = CheckboxOption::Simple();

    option.on_change = [forceFocus]() mutable
    {
        *forceFocus = true;
    };
    option.transform = [](const EntryState &s) -> Element
    {
#if 0
        // Microsoft terminal do not use fonts able to render properly the default
        // radiobox glyph.
        auto prefix = text(s.state ? "[X] " : "[ ] "); // NOLINT
#else
        auto prefix = text(s.state ? "▣ " : "☐ "); // NOLINT
#endif
        auto t = text(s.label);
        if (s.active != s.focused)
        {
            s.label;
        }
        if (s.active)
        {
            t = t | color(Color::White);
        }
        if (s.focused)
        {
            t = t | color(Color::White) | bgcolor(Color::GrayDark);
        }

        return hbox({prefix, t});
    };
    return option;
}
CheckboxOption CustomRadioCheckboxOption(bool *selections, size_t size, size_t index, int *forceFocus)
{
    CheckboxOption option = CheckboxOption::Simple();

    option.on_change = [selections, size, index, forceFocus]() mutable
    {
        *forceFocus = index;
        for (size_t i = 0; i < size; ++i)
        {
            selections[i] = (i == index);
        }
    };
    option.transform = [](const EntryState &s) -> Element
    {
#if defined(FTXUI_MICROSOFT_TERMINAL_FALLBACK)
        // Microsoft terminal do not use fonts able to render properly the default
        // radiobox glyph.
        auto prefix = text(s.state ? "(*) " : "( ) "); // NOLINT
#else
        auto prefix = text(s.state ? "◉ " : "○ "); // NOLINT
#endif
        auto t = text(s.label);
        if (s.active != s.focused)
        {
            s.label;
        }
        if (s.active)
        {
            t = t | color(Color::White);
        }
        if (s.focused)
        {
            t = t | color(Color::White) | bgcolor(Color::GrayDark);
        }

        return hbox({prefix, t});
    };
    return option;
}

std::unique_ptr<std::jthread> setBootParametersThread;

void setBootParameters(std::shared_ptr<BootConfig> newConfig, std::function<void(bool success, std::string errorMessage)> onComplete)
{
    newConfig->WriteConfiguration(onComplete);
}

void kconfigUi(void)
{
    const std::string kernelType = "PREEMPT_DYNAMIC";

    int kernelMode = 0;

    const int NO_DIALOG = 0;
    const int PROCESSING_DIALOG = 1;
    const int REBOOT_DIALOG = 2;
    const int ERROR_DIALOG = 3;

    Clock::time_point startTime = Clock::now();

    int dialogIndex = 0;
    std::string errorMessage;

    auto screen = ScreenInteractive::Fullscreen();

    auto quit = [&screen]()
    {
        screen.Exit();
    };

    std::mutex updateCallbackMutex;
    bool updatePending = false;
    bool updateComplete = false;
    std::string updateErrorMessage;

    auto onError = [&errorMessage, &dialogIndex](const std::string &error) mutable
    {
        errorMessage = error;
        dialogIndex = ERROR_DIALOG;
    };

    bool useThreadedIrqs = true;

    BootConfig::DynamicSchedulerT dynamicScheduler = BootConfig::DynamicSchedulerT::NotApplicable;
    bool threadedIrqsAvailable = false;
    bool configRequired = false;

    std::unique_ptr<BootConfig> bootConfig;
    using DynamicSchedulerT = BootConfig::DynamicSchedulerT;
    try
    {
        bootConfig = std::make_unique<BootConfig>();
        threadedIrqsAvailable = bootConfig->CanSetThreadIrqs();
        dynamicScheduler = bootConfig->DynamicScheduler();
        useThreadedIrqs = bootConfig->ThreadedIrqs();
        configRequired = bootConfig->DynamicScheduler() != DynamicSchedulerT::NotApplicable || useThreadedIrqs;

        if (!bootConfig->CanWriteConfig())
        {
            throw std::runtime_error("Unrecognized boot loader. Unable to reconfigure the kernel.");
        }
    }
    catch (const std::exception &e)
    {
        onError(e.what());
    }

    bool forceThreadedIrqFocus = false;


    int dynamicSchedulerIndex = (int)dynamicScheduler;
    std::vector<ConstStringRef> preemptModeLabelRefs = {
        " none               ",
        " voluntary          ",
        " full (recommended) "};
    bool preemptModeSelections[3]; // std::vector<bool> is bizarrely specialized, so it doesn't wrok.


    for (int i = 0; i < 3; ++i)
    {
        preemptModeSelections[i] = (i == dynamicSchedulerIndex);
    }



    int forceSchedulerFocus = -1;

    Component preemptCheckBox0 =
        Checkbox(
            preemptModeLabelRefs[0],
            &(preemptModeSelections[0]),
            CustomRadioCheckboxOption(preemptModeSelections, 3, 0, &forceSchedulerFocus));
    Component preemptCheckBox1 =
        Checkbox(
            preemptModeLabelRefs[1],
            &(preemptModeSelections[1]),
            CustomRadioCheckboxOption(preemptModeSelections, 3, 1, &forceSchedulerFocus));
    Component preemptCheckBox2 =
        Checkbox(
            preemptModeLabelRefs[2],
            &(preemptModeSelections[2]),
            CustomRadioCheckboxOption(preemptModeSelections, 3, 2, &forceSchedulerFocus));

    std::vector<Component> preemptCheckboxes{
        preemptCheckBox0,
        preemptCheckBox1,
        preemptCheckBox2,
    };

    ConstStringRef checkboxRef{" Enable threadirqs (recommended)    "};
    Component threadedIrqCheckbox = Checkbox(checkboxRef, &useThreadedIrqs, CustomCheckboxOption(&forceThreadedIrqFocus));

    // The basic input components:
    auto handleSave = [&]()
    {
        if (updatePending)
        {
            onError("Save already running.");
            return;
        }

        dialogIndex = PROCESSING_DIALOG;

        std::shared_ptr<BootConfig> newBootConfig = std::make_shared<BootConfig>();

        if (bootConfig->CanSetThreadIrqs())
        {
            newBootConfig->ThreadedIrqs(useThreadedIrqs);
        }
        if (bootConfig->DynamicScheduler() != DynamicSchedulerT::NotApplicable)
        {
            int schedulerIndex = -1;
            for (size_t i = 0; i < 3; ++i)
            {
                if (preemptModeSelections[i])
                {
                    schedulerIndex = i;
                    break;
                }
            }
            if (schedulerIndex == -1)
            {
                onError("Internal state error.");
            }
            else
            {
                newBootConfig->DynamicScheduler((DynamicSchedulerT)schedulerIndex);

                if (newBootConfig->Changed())
                {                
                    {
                        std::lock_guard lock{updateCallbackMutex};
                        updatePending = true;
                        updateComplete = false;
                        updateErrorMessage = true;
                    }

                    newBootConfig->WriteConfiguration(
                        [&](bool success, std::string errorMessage)
                        {
                            // potentially wrong-threaded. We pick up the result because we're animating the "..."
                            std::lock_guard lock(updateCallbackMutex);
                            updateComplete = true;
                            updateErrorMessage = errorMessage;
                        });
                } else {
                    // no changes. Just quit.
                    quit();
                }
            }
        }
    };

    Component okButton = Button(
        ConstStringRef("  OK  "),
        [&]() mutable
        {
            screen.ExitLoopClosure()();
        },
        ButtonOption::Ascii());

    Component saveButton = Button(
        ConstStringRef("  Save  "),
        [&]() mutable
        {
            handleSave();
        },
        ButtonOption::Ascii());

    Component cancelButton = Button(
        ConstStringRef(" Cancel "),
        [&screen]() mutable
        {
            screen.ExitLoopClosure()();
        },
        ButtonOption::Ascii());

    // The component tree:
    Component tabOrder;

    if (configRequired)
    {
        Components components;
        if (dynamicScheduler != DynamicSchedulerT::NotApplicable)
        {
            components.push_back(preemptCheckBox0);
            components.push_back(preemptCheckBox1);
            components.push_back(preemptCheckBox2);
        }
        if (threadedIrqsAvailable)
        {
            components.push_back(threadedIrqCheckbox);
        }
        components.push_back(Container::Horizontal({cancelButton, saveButton}));

        tabOrder = Container::Vertical(components);
    }
    else
    {
        tabOrder = Container::Vertical({okButton});
    }

    // component tree for Processing dialog.
    auto focusTrap = Container::Horizontal({Button(ButtonOption::Ascii())});

    auto mainRenderer = Renderer(
        tabOrder,
        [&]() mutable
        {
            if (forceThreadedIrqFocus)
            {
                threadedIrqCheckbox->TakeFocus();
                forceThreadedIrqFocus = false;
            }
            if (forceSchedulerFocus >= 0)
            {
                preemptCheckboxes[forceSchedulerFocus]->TakeFocus();
                forceSchedulerFocus = -1;
            }
            Elements topLevelElements;

            Elements contentElements;
            {
                contentElements.push_back(hbox(text("Kernel type: "), text(kernelType)));
                contentElements.push_back(text(""));

                if (!configRequired)
                {
                    contentElements.push_back(text("No configuration required."));
                }
                else
                {
                    Elements kernelCommandLineOptions;
                    {
                        kernelCommandLineOptions.push_back(text(""));
                        if (dynamicScheduler != DynamicSchedulerT::NotApplicable)
                        {
                            kernelCommandLineOptions.push_back(text("     Preempt"));
                            kernelCommandLineOptions.push_back(hbox(text("       "), preemptCheckBox0->Render(), text("     ")));
                            kernelCommandLineOptions.push_back(hbox(text("       "), preemptCheckBox1->Render(), text("     ")));
                            kernelCommandLineOptions.push_back(hbox(text("       "), preemptCheckBox2->Render(), text("     ")));
                        }
                        if (dynamicScheduler != DynamicSchedulerT::NotApplicable && threadedIrqsAvailable)
                        {
                            kernelCommandLineOptions.push_back(text(""));
                        }
                        if (threadedIrqsAvailable)
                        {
                            kernelCommandLineOptions.push_back(hbox(text("     "), threadedIrqCheckbox->Render(), text("     ")));
                        }
                        kernelCommandLineOptions.push_back(text(""));
                    }
                    contentElements.push_back(
                        window(
                            text(" Kernel Commandline Options "),
                            vbox(kernelCommandLineOptions)));
                }
                auto dialog = vbox(
                                  {text("  PiPedal Kernel Configuration"),
                                   separator(),
                                   text(""),
                                   hbox(text("      "), vbox(contentElements) | color(Color::White), text("      ")),
                                   text(""),
                                   text(""),
                                   configRequired ? hbox(text("") | flex, cancelButton->Render(), saveButton->Render(), text(" "))
                                                  : hbox(text("") | flex, okButton->Render(), text(" "))}) |
                              (borderStyled(BorderStyle::HEAVY) | color(Color::White));
                topLevelElements.push_back(std::move(center(dialog)));
            }
            return dbox(topLevelElements);
        });
    mainRenderer = CatchEvent(
        mainRenderer,
        [&](Event event)
        {
            if (event.is_character())
            {
                if (event == Event::Character('q'))
                {
                    screen.ExitLoopClosure()();
                    return true;
                }
            }
            return false;
        });
    auto processDialogRenderer = Renderer(focusTrap, [&]()
                                          {
            auto dialog = vbox(
                {
                    text("  Applying changes..."),
                    hbox(text("  "),text(spinnerText(startTime)),text("  "))
                }) | borderHeavy | clear_under | center;

            return dialog; });

    ConstStringRef rebootNowText("   Now   ");
    ConstStringRef rebootLaterText("  Later  ");

    auto rebootNowButton = Button(
        rebootNowText,
        [&]()
        {
            dialogIndex = 0;
            silentSysExec("sudo reboot now");
            quit();
        },
        ButtonOption::Ascii());
    auto rebootLaterButton = Button(
        rebootLaterText,
        [&]()
        {
            dialogIndex = 0;
            quit();
        },
        ButtonOption::Ascii());
    auto rebootDialogContainer = Container::Horizontal({rebootLaterButton, rebootNowButton});

    auto rebootDialogRenderer = Renderer(
        rebootDialogContainer,
        [&]()
        {
            return hbox({text("  "),
                         vbox({hbox(text("    "), paragraph("Reboot required. Would you like to reboot now or later?") | size(WIDTH, LESS_THAN, 35), text("    ")),
                               text(""),
                               hbox(text("") | flex, rebootLaterButton->Render(), rebootNowButton->Render(), text("  ")),
                               text("  ")})}) |
                   borderHeavy;
        });

    ////////////// ErrorDialog  /////////////////////////////////////////////////

    ConstStringRef errorOkText("   OK   ");

    auto errorOkButton = Button(
        errorOkText,
        [&]()
        {
            dialogIndex = 0;
            quit();
        },
        ButtonOption::Ascii());
    auto errorDialogContainer = Container::Horizontal({errorOkButton});

    auto errorDialogRenderer = Renderer(
        errorDialogContainer,
        [&]()
        {
            return vbox(
                       {text("  Error"),
                        separator(),

                        vbox({hbox(text("   "), paragraph(errorMessage) | size(WIDTH, LESS_THAN, 35), text("    ")),
                              text(""),
                              hbox(text("") | flex, errorOkButton->Render(), text("  "))

                        })}) |
                   borderHeavy;
        });

    /////////////////////////////////////////////////////////////////////////////

    auto modalDialogContainer = Container::Tab(
        {

            mainRenderer,
            processDialogRenderer,
            rebootDialogRenderer,
            errorDialogRenderer},
        &dialogIndex);

    auto modalDialogRenderer = Renderer(
        modalDialogContainer,
        [&]() mutable
        {
            Element document = modalDialogContainer->ChildAt(0)->Render();
            if (dialogIndex != 0)
            {
                document = dbox(
                    document,
                    modalDialogContainer->ChildAt(dialogIndex)->Render() | clear_under | center);
            }
            if (dialogIndex == PROCESSING_DIALOG)
            {
                screen.RequestAnimationFrame();
                {
                    std::lock_guard lock(updateCallbackMutex);
                    if (updatePending && updateComplete)
                    {
                        updatePending = false;
                        updateComplete = false;
                        if (updateErrorMessage.length() != 0)
                        {
                            onError(updateErrorMessage);
                            updateErrorMessage = "";
                        } else {
                            // operation complete. 
                            // show the reboot dialog.
                            dialogIndex = REBOOT_DIALOG;

                        }

                    }
                }

            }
            return document;
        });

    screen.Loop(modalDialogRenderer);
}


void printHelp()
{
    using namespace std;

    PrettyPrinter pp;


    pp << 
        "pipedal_kconfig - Edit kernel commandline arguments. \n" 
        << "Copyright (c) Robin E. R. Davies"
        << "\n\n"
        << "A utility that provides a user-friendly interface for configuring  kernel commandline arguments that affect realtime audio performance on PREEMPT_DYANMIC kernels (like Ubuntu)."
        << "\n\n"
        << "Linux kernels that have PREEMPT_DYNAMIC kernels provide excellent support for real-time low-latency audio, but are not "
        << "optimially configured for real-time audio by default. "
        << "\n\n"
        << "To provide best performance, the following two kernel commandline arguments need to be set:"
        << "\n\n"
        << Indent(20)
        << HangingIndent()
        << "   preempt=full\tallows the kernel to preempt kernel threads` in order to service more urgent real-time audio tasks."
        << "\n\n"
        << HangingIndent()
        << "   threadirqs\tallows realtime audio tasks to run with higher priority than network and hard disk interrupt handlers."
        << "\n\n"
        << Indent(0)
        << "These settings have no effect on PREEMT_RT kernels (like Raspberry Pi OS), since PREEMPT_RT kernels turn these features on by default."
        << "\n\n"
        << "pipedal_kconfig needs to run with root privileges."
        << "\n\n"
        ;


}

int main(int argc, char**argv)
{
    bool help = false;
    bool noSudo = false;
    CommandLineParser cmdline;
    cmdline.AddOption("-h",&help);
    cmdline.AddOption("--help",&help);
    cmdline.AddOption("--no-sudo",&noSudo); // undocumented option to allow debugging of most of the code, except for the finial privileged bits.


    try {
        cmdline.Parse(argc,(const char**)argv);

        if (help || cmdline.Arguments().size() != 0) 
        {
            printHelp();
            return EXIT_SUCCESS;
        }

    } catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    auto uid = getuid();
    if (uid != 0 && !noSudo)
    {
        return SudoExec(argc, argv);
    }


    kconfigUi();
    return EXIT_SUCCESS;
}