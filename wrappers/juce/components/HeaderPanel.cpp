//
// Created by Paul Walker on 1/5/22.
//

#include "HeaderPanel.h"
#include "widgets/RaisedToggleButton.h"
#include "widgets/OutlinedTextButton.h"
#include "widgets/CompactVUMeter.h"

namespace SC3
{
namespace Components
{
HeaderPanel::HeaderPanel(SC3Editor *ed) : editor(ed)
{
    for (int i = 0; i < n_sampler_parts; ++i)
    {
        auto b = std::make_unique<SC3::Widgets::RaisedToggleButton>(std::to_string(i));
        b->setClickingTogglesState(true);
        b->setRadioGroupId(174, juce::NotificationType::dontSendNotification);
        addAndMakeVisible(*b);
        partsButtons[i] = std::move(b);
        partsButtons[i]->onClick = [this, i]() {
            if (partsButtons[i]->getToggleState())
                editor->selectPart(i);
        };
    }
    partsButtons[editor->selectedPart]->setToggleState(
        true, juce::NotificationType::dontSendNotification);

    zonesButton = std::make_unique<SC3::Widgets::OutlinedTextButton>("Zones");
    zonesButton->setClickingTogglesState(true);
    zonesButton->setRadioGroupId(175, juce::NotificationType::dontSendNotification);
    zonesButton->onClick = [this]() { editor->showPage(SC3Editor::ZONE); };
    zonesButton->setToggleState(true, juce::NotificationType::dontSendNotification);
    addAndMakeVisible(*zonesButton);

    partButton = std::make_unique<SC3::Widgets::OutlinedTextButton>("Part");
    partButton->setClickingTogglesState(true);
    partButton->setRadioGroupId(175, juce::NotificationType::dontSendNotification);
    partButton->onClick = [this]() { editor->showPage(SC3Editor::PART); };
    addAndMakeVisible(*partButton);

    fxButton = std::make_unique<SC3::Widgets::OutlinedTextButton>("FX");
    fxButton->setClickingTogglesState(true);
    fxButton->setRadioGroupId(175, juce::NotificationType::dontSendNotification);
    fxButton->onClick = [this]() { editor->showPage(SC3Editor::FX); };
    addAndMakeVisible(*fxButton);

    configButton = std::make_unique<SC3::Widgets::OutlinedTextButton>("Config");
    configButton->setClickingTogglesState(true);
    configButton->setRadioGroupId(175, juce::NotificationType::dontSendNotification);
    configButton->onClick = [this]() { editor->showPage(SC3Editor::CONFIG); };
    addAndMakeVisible(*configButton);

    aboutButton = std::make_unique<SC3::Widgets::OutlinedTextButton>("About");
    aboutButton->setClickingTogglesState(true);
    aboutButton->setRadioGroupId(175, juce::NotificationType::dontSendNotification);
    aboutButton->onClick = [this]() { editor->showPage(SC3Editor::ABOUT); };
    addAndMakeVisible(*aboutButton);

    menuButton = std::make_unique<SC3::Widgets::OutlinedTextButton>("Menu");
    addAndMakeVisible(*menuButton);

    vuMeter0 = std::make_unique<SC3::Widgets::CompactVUMeter>(editor);
    addAndMakeVisible(*vuMeter0);
}

HeaderPanel::~HeaderPanel() {}

void HeaderPanel::resized()
{
    auto r = getLocalBounds().reduced(2, 2);
    r = r.withWidth(r.getHeight());

    for (int i = 0; i < n_sampler_parts; ++i)
    {
        partsButtons[i]->setBounds(r);
        r = r.translated(r.getHeight(), 0);
    }

    auto partR = r.getRight();

    auto nRButtons = 6; // zones part fx config menu
    auto rbWidth = 50;
    auto margin = 5;
    r = getLocalBounds().reduced(2, 2);
    r = r.withLeft(r.getRight() - nRButtons * (rbWidth + margin) + margin);
    r = r.withWidth(rbWidth);

    auto buttonL = r.getX();

    zonesButton->setBounds(r);
    r = r.translated(rbWidth + margin, 0);
    partButton->setBounds(r);
    r = r.translated(rbWidth + margin, 0);
    fxButton->setBounds(r);
    r = r.translated(rbWidth + margin, 0);
    configButton->setBounds(r);
    r = r.translated(rbWidth + margin, 0);
    aboutButton->setBounds(r);
    r = r.translated(rbWidth + margin, 0);
    menuButton->setBounds(r);
    r = r.translated(rbWidth + margin, 0);

    r = getLocalBounds().withWidth(128).withCentre({getWidth() / 2, getHeight() / 2}).reduced(0, 2);
    vuMeter0->setBounds(r);
}

void HeaderPanel::onProxyUpdate()
{
    if (editor->selectedPart >= 0 && editor->selectedPart < n_sampler_parts)
    {
        partsButtons[editor->selectedPart]->setToggleState(
            true, juce::NotificationType::dontSendNotification);
    }
}
} // namespace Components
} // namespace SC3