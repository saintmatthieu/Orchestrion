/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2024 Matthieu Hodgkinson
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "ComputerKeyboardMidiController.h"
#include "IOrchestrionSequencer.h"

namespace dgk
{
void ComputerKeyboardMidiController::init()
{
  keyboard()->keyPressed().onReceive(this, [this](char letter)
                                     { keyPressed(letter); });
  keyboard()->keyReleased().onReceive(this, [this](char letter)
                                      { keyReleased(letter); });
  updateNoteMap();
  keyboard()->layoutChanged().onNotify(this, [this] { updateNoteMap(); });
}

void ComputerKeyboardMidiController::updateNoteMap()
{
  std::unordered_map<char, Note> usLayout{
      // row 1
      {'1', {0, 1.0f}},
      {'2', {1, 1.0f}},
      {'3', {3, 1.0f}},
      {'4', {4, 1.0f}},
      {'5', {5, 1.0f}},
      {'6', {60, 1.0f}},
      {'7', {61, 1.0f}},
      {'8', {62, 1.0f}},
      {'9', {63, 1.0f}},
      {'0', {64, 1.0f}},
      // row 2
      {'q', {6, .75f}},
      {'w', {7, .75f}},
      {'e', {8, .75f}},
      {'r', {9, .75f}},
      {'y', {65, .75f}},
      {'u', {66, .75f}},
      {'i', {67, .75f}},
      {'o', {68, .75f}},
      // row 3
      {'a', {11, .50f}},
      {'s', {12, .50f}},
      {'d', {13, .50f}},
      {'f', {14, .50f}},
      {'g', {69, .50f}},
      {'h', {70, .50f}},
      {'j', {71, .50f}},
      {'k', {72, .50f}},
      // row 4
      {'z', {15, .25f}},
      {'x', {16, .25f}},
      {'c', {17, .25f}},
      {'b', {73, .25f}},
      {'n', {74, .25f}},
      {'m', {75, .25f}},
  };

  switch (keyboard()->layout())
  {
  case IComputerKeyboard::Layout::US:
    m_noteMap = std::move(usLayout);
    break;
  case IComputerKeyboard::Layout::German:
  default:
    m_noteMap = std::move(usLayout);
    // Same as US, just swap y and z.
    const auto tmp = m_noteMap.at('y');
    m_noteMap.emplace('y', m_noteMap.at('z'));
    m_noteMap.emplace('z', tmp);
    break;
  }
}

void ComputerKeyboardMidiController::keyPressed(char letter)
{
  letter = std::tolower(letter);

  if (std::find_if(m_noteMap.begin(), m_noteMap.end(),
                   [letter](const auto &note)
                   { return note.first == letter; }) == m_noteMap.end())
    return;

  if (m_pressedLetters.count(letter) > 0)
    // An auto-repeat event probably, the user keeping the finger pressed on
    // a key.
    return;

  const auto sequencer = orchestrion()->sequencer();
  if (!sequencer)
    return;

  NoteEvent evt;
  evt.type = NoteEvent::Type::noteOn;
  evt.pitch = m_noteMap.at(letter).pitch;
  evt.velocity = m_noteMap.at(letter).velocity;
  m_pressedLetters.insert(letter);
  sequencer->OnInputEvent(evt);
}

void ComputerKeyboardMidiController::keyReleased(char letter)
{
  letter = std::tolower(letter);

  if (std::find_if(m_noteMap.begin(), m_noteMap.end(),
                   [letter](const auto &note)
                   { return note.first == letter; }) == m_noteMap.end())
    return;

  const auto sequencer = orchestrion()->sequencer();
  if (!sequencer)
    return;

  m_pressedLetters.erase(letter);
  NoteEvent evt;
  evt.type = NoteEvent::Type::noteOff;
  evt.velocity = 0.f;
  evt.pitch = m_noteMap.at(letter).pitch;
  sequencer->OnInputEvent(evt);
}
} // namespace dgk