/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-11 by Raw Material Software Ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the GNU General
   Public License (Version 2), as published by the Free Software Foundation.
   A copy of the license is included in the JUCE distribution, or can be found
   online at www.gnu.org/licenses.

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.rawmaterialsoftware.com/juce for more information.

  ==============================================================================
*/

class DocumentWindow::ButtonListenerProxy  : public ButtonListener // (can't use Button::Listener due to idiotic VC2005 bug)
{
public:
    ButtonListenerProxy (DocumentWindow& owner_) : owner (owner_) {}

    void buttonClicked (Button* button)
    {
        if      (button == owner.getMinimiseButton())  owner.minimiseButtonPressed();
        else if (button == owner.getMaximiseButton())  owner.maximiseButtonPressed();
        else if (button == owner.getCloseButton())     owner.closeButtonPressed();
    }

private:
    DocumentWindow& owner;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ButtonListenerProxy);
};

//==============================================================================
DocumentWindow::DocumentWindow (const String& title,
                                const Colour& backgroundColour,
                                const int requiredButtons_,
                                const bool addToDesktop_)
    : ResizableWindow (title, backgroundColour, addToDesktop_),
      titleBarHeight (26),
      menuBarHeight (24),
      requiredButtons (requiredButtons_),
     #if JUCE_MAC
      positionTitleBarButtonsOnLeft (true),
     #else
      positionTitleBarButtonsOnLeft (false),
     #endif
      drawTitleTextCentred (true),
      menuBarModel (nullptr)
{
    setResizeLimits (128, 128, 32768, 32768);

    DocumentWindow::lookAndFeelChanged();
}

DocumentWindow::~DocumentWindow()
{
    // Don't delete or remove the resizer components yourself! They're managed by the
    // DocumentWindow, and you should leave them alone! You may have deleted them
    // accidentally by careless use of deleteAllChildren()..?
    jassert (menuBar == nullptr || getIndexOfChildComponent (menuBar) >= 0);
    jassert (titleBarButtons[0] == nullptr || getIndexOfChildComponent (titleBarButtons[0]) >= 0);
    jassert (titleBarButtons[1] == nullptr || getIndexOfChildComponent (titleBarButtons[1]) >= 0);
    jassert (titleBarButtons[2] == nullptr || getIndexOfChildComponent (titleBarButtons[2]) >= 0);

    for (int i = numElementsInArray (titleBarButtons); --i >= 0;)
        titleBarButtons[i] = nullptr;

    menuBar = nullptr;
}

//==============================================================================
void DocumentWindow::repaintTitleBar()
{
    repaint (getTitleBarArea());
}

void DocumentWindow::setName (const String& newName)
{
    if (newName != getName())
    {
        Component::setName (newName);
        repaintTitleBar();
    }
}

void DocumentWindow::setIcon (const Image& imageToUse)
{
    titleBarIcon = imageToUse;
    repaintTitleBar();
}

void DocumentWindow::setTitleBarHeight (const int newHeight)
{
    titleBarHeight = newHeight;
    resized();
    repaintTitleBar();
}

void DocumentWindow::setTitleBarButtonsRequired (const int requiredButtons_,
                                                 const bool positionTitleBarButtonsOnLeft_)
{
    requiredButtons = requiredButtons_;
    positionTitleBarButtonsOnLeft = positionTitleBarButtonsOnLeft_;
    lookAndFeelChanged();
}

void DocumentWindow::setTitleBarTextCentred (const bool textShouldBeCentred)
{
    drawTitleTextCentred = textShouldBeCentred;
    repaintTitleBar();
}

//==============================================================================
void DocumentWindow::setMenuBar (MenuBarModel* newMenuBarModel, const int newMenuBarHeight)
{
    if (menuBarModel != newMenuBarModel)
    {
        menuBar = nullptr;

        menuBarModel = newMenuBarModel;
        menuBarHeight = newMenuBarHeight > 0 ? newMenuBarHeight
                                             : getLookAndFeel().getDefaultMenuBarHeight();

        if (menuBarModel != nullptr)
            setMenuBarComponent (new MenuBarComponent (menuBarModel));

        resized();
    }
}

Component* DocumentWindow::getMenuBarComponent() const noexcept
{
    return menuBar;
}

void DocumentWindow::setMenuBarComponent (Component* newMenuBarComponent)
{
    // (call the Component method directly to avoid the assertion in ResizableWindow)
    Component::addAndMakeVisible (menuBar = newMenuBarComponent);

    if (menuBar != nullptr)
        menuBar->setEnabled (isActiveWindow());

    resized();
}

//==============================================================================
void DocumentWindow::closeButtonPressed()
{
    /*  If you've got a close button, you have to override this method to get
        rid of your window!

        If the window is just a pop-up, you should override this method and make
        it delete the window in whatever way is appropriate for your app. E.g. you
        might just want to call "delete this".

        If your app is centred around this window such that the whole app should quit when
        the window is closed, then you will probably want to use this method as an opportunity
        to call JUCEApplication::quit(), and leave the window to be deleted later by your
        JUCEApplication::shutdown() method. (Doing it this way means that your window will
        still get cleaned-up if the app is quit by some other means (e.g. a cmd-Q on the mac
        or closing it via the taskbar icon on Windows).
    */
    jassertfalse;
}

void DocumentWindow::minimiseButtonPressed()
{
    setMinimised (true);
}

void DocumentWindow::maximiseButtonPressed()
{
    setFullScreen (! isFullScreen());
}

//==============================================================================
void DocumentWindow::paint (Graphics& g)
{
    ResizableWindow::paint (g);

    if (resizableBorder == nullptr)
    {
        g.setColour (getBackgroundColour().overlaidWith (Colour (0x80000000)));

        const BorderSize<int> border (getBorderThickness());

        g.fillRect (0, 0, getWidth(), border.getTop());
        g.fillRect (0, border.getTop(), border.getLeft(), getHeight() - border.getTopAndBottom());
        g.fillRect (getWidth() - border.getRight(), border.getTop(), border.getRight(), getHeight() - border.getTopAndBottom());
        g.fillRect (0, getHeight() - border.getBottom(), getWidth(), border.getBottom());
    }

    const Rectangle<int> titleBarArea (getTitleBarArea());
    g.reduceClipRegion (titleBarArea);
    g.setOrigin (titleBarArea.getX(), titleBarArea.getY());

    int titleSpaceX1 = 6;
    int titleSpaceX2 = titleBarArea.getWidth() - 6;

    for (int i = 0; i < 3; ++i)
    {
        if (titleBarButtons[i] != nullptr)
        {
            if (positionTitleBarButtonsOnLeft)
                titleSpaceX1 = jmax (titleSpaceX1, titleBarButtons[i]->getRight() + (getWidth() - titleBarButtons[i]->getRight()) / 8);
            else
                titleSpaceX2 = jmin (titleSpaceX2, titleBarButtons[i]->getX() - (titleBarButtons[i]->getX() / 8));
        }
    }

    getLookAndFeel().drawDocumentWindowTitleBar (*this, g,
                                                 titleBarArea.getWidth(),
                                                 titleBarArea.getHeight(),
                                                 titleSpaceX1,
                                                 jmax (1, titleSpaceX2 - titleSpaceX1),
                                                 titleBarIcon.isValid() ? &titleBarIcon : 0,
                                                 ! drawTitleTextCentred);
}

void DocumentWindow::resized()
{
    ResizableWindow::resized();

    if (titleBarButtons[1] != nullptr)
        titleBarButtons[1]->setToggleState (isFullScreen(), false);

    const Rectangle<int> titleBarArea (getTitleBarArea());

    getLookAndFeel()
        .positionDocumentWindowButtons (*this,
                                        titleBarArea.getX(), titleBarArea.getY(),
                                        titleBarArea.getWidth(), titleBarArea.getHeight(),
                                        titleBarButtons[0],
                                        titleBarButtons[1],
                                        titleBarButtons[2],
                                        positionTitleBarButtonsOnLeft);

    if (menuBar != nullptr)
        menuBar->setBounds (titleBarArea.getX(), titleBarArea.getBottom(),
                            titleBarArea.getWidth(), menuBarHeight);
}

BorderSize<int> DocumentWindow::getBorderThickness()
{
    return BorderSize<int> ((isFullScreen() || isUsingNativeTitleBar())
                                ? 0 : (resizableBorder != nullptr ? 4 : 1));
}

BorderSize<int> DocumentWindow::getContentComponentBorder()
{
    BorderSize<int> border (getBorderThickness());

    border.setTop (border.getTop()
                    + (isUsingNativeTitleBar() ? 0 : titleBarHeight)
                    + (menuBar != nullptr ? menuBarHeight : 0));

    return border;
}

int DocumentWindow::getTitleBarHeight() const
{
    return isUsingNativeTitleBar() ? 0 : jmin (titleBarHeight, getHeight() - 4);
}

Rectangle<int> DocumentWindow::getTitleBarArea()
{
    const BorderSize<int> border (getBorderThickness());

    return Rectangle<int> (border.getLeft(), border.getTop(),
                           getWidth() - border.getLeftAndRight(), getTitleBarHeight());
}

Button* DocumentWindow::getCloseButton()    const noexcept  { return titleBarButtons[2]; }
Button* DocumentWindow::getMinimiseButton() const noexcept  { return titleBarButtons[0]; }
Button* DocumentWindow::getMaximiseButton() const noexcept  { return titleBarButtons[1]; }

int DocumentWindow::getDesktopWindowStyleFlags() const
{
    int styleFlags = ResizableWindow::getDesktopWindowStyleFlags();

    if ((requiredButtons & minimiseButton) != 0)  styleFlags |= ComponentPeer::windowHasMinimiseButton;
    if ((requiredButtons & maximiseButton) != 0)  styleFlags |= ComponentPeer::windowHasMaximiseButton;
    if ((requiredButtons & closeButton)    != 0)  styleFlags |= ComponentPeer::windowHasCloseButton;

    return styleFlags;
}

void DocumentWindow::lookAndFeelChanged()
{
    for (int i = numElementsInArray (titleBarButtons); --i >= 0;)
        titleBarButtons[i] = nullptr;

    if (! isUsingNativeTitleBar())
    {
        LookAndFeel& lf = getLookAndFeel();

        if ((requiredButtons & minimiseButton) != 0)  titleBarButtons[0] = lf.createDocumentWindowButton (minimiseButton);
        if ((requiredButtons & maximiseButton) != 0)  titleBarButtons[1] = lf.createDocumentWindowButton (maximiseButton);
        if ((requiredButtons & closeButton)    != 0)  titleBarButtons[2] = lf.createDocumentWindowButton (closeButton);

        for (int i = 0; i < 3; ++i)
        {
            if (titleBarButtons[i] != nullptr)
            {
                if (buttonListener == nullptr)
                    buttonListener = new ButtonListenerProxy (*this);

                titleBarButtons[i]->addListener (buttonListener);
                titleBarButtons[i]->setWantsKeyboardFocus (false);

                // (call the Component method directly to avoid the assertion in ResizableWindow)
                Component::addAndMakeVisible (titleBarButtons[i]);
            }
        }

        if (getCloseButton() != nullptr)
        {
           #if JUCE_MAC
            getCloseButton()->addShortcut (KeyPress ('w', ModifierKeys::commandModifier, 0));
           #else
            getCloseButton()->addShortcut (KeyPress (KeyPress::F4Key, ModifierKeys::altModifier, 0));
           #endif
        }
    }

    activeWindowStatusChanged();

    ResizableWindow::lookAndFeelChanged();
}

void DocumentWindow::parentHierarchyChanged()
{
    lookAndFeelChanged();
}

void DocumentWindow::activeWindowStatusChanged()
{
    ResizableWindow::activeWindowStatusChanged();

    for (int i = numElementsInArray (titleBarButtons); --i >= 0;)
        if (titleBarButtons[i] != nullptr)
            titleBarButtons[i]->setEnabled (isActiveWindow());

    if (menuBar != nullptr)
        menuBar->setEnabled (isActiveWindow());
}

void DocumentWindow::mouseDoubleClick (const MouseEvent& e)
{
    Button* const maximise = getMaximiseButton();

    if (maximise != nullptr && getTitleBarArea().contains (e.x, e.y))
        maximise->triggerClick();
}

void DocumentWindow::userTriedToCloseWindow()
{
    closeButtonPressed();
}
