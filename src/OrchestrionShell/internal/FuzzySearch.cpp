/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2026 Matthieu Hodgkinson
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
#include "FuzzySearch.h"

#include <QStringList>
#include <algorithm>
#include <limits>
#include <vector>

namespace dgk
{
namespace
{
// The score of a query word's letter, by where it falls in the text.
constexpr int letterPoints = 1;
constexpr int wordStartBonus = 8;
constexpr int adjacentBonus = 5;
/** Per letter of the text skipped between two of the word's letters. */
constexpr int gapPenalty = 1;

constexpr int unmatched = std::numeric_limits<int>::min() / 2;

/**
 * The best score of `word` against `text`, if all its letters are found in
 * order. Dynamic programming over (letter of the word, position in the
 * text): best[i] is the best score of the word's letters so far with the
 * last of them at text position i. Letters strewn far apart cost more than
 * they bring, so that a match must hang together to count: one that scores
 * less than a point a letter is none.
 */
std::optional<int> wordScore(const QString &word, const QString &text)
{
  const auto n = text.size();
  std::vector<int> best(n, unmatched);
  std::vector<int> next(n);
  for (qsizetype j = 0; j < word.size(); ++j)
  {
    // The best score with the previous letter anywhere before i - 1, i.e.
    // with a gap, less what the gap costs.
    int bestBefore = unmatched;
    for (qsizetype i = 0; i < n; ++i)
    {
      next[i] = unmatched;
      if (text[i] == word[j])
      {
        const bool wordStart = i == 0 || text[i - 1] == ' ';
        const int here = letterPoints + (wordStart ? wordStartBonus : 0);
        if (j == 0)
          next[i] = here;
        else
        {
          const int adjacent = i > 0 && best[i - 1] != unmatched
                                   ? best[i - 1] + adjacentBonus
                                   : unmatched;
          const int gapped = bestBefore;
          const int previous = std::max(adjacent, gapped);
          if (previous != unmatched)
            next[i] = previous + here;
        }
      }
      if (i > 0)
        bestBefore = std::max(
            bestBefore == unmatched ? unmatched : bestBefore - gapPenalty,
            best[i - 1] == unmatched ? unmatched : best[i - 1] - gapPenalty);
    }
    std::swap(best, next);
  }
  const int score =
      n > 0 ? *std::max_element(best.begin(), best.end()) : unmatched;
  if (score < word.size() * letterPoints)
    return std::nullopt;
  return score;
}
} // namespace

QString foldForSearch(const QString &text)
{
  QString folded;
  folded.reserve(text.size());
  // Decomposed, the accents are separate marks, which are dropped.
  for (const QChar c : text.normalized(QString::NormalizationForm_KD))
  {
    if (c.isMark())
      continue;
    folded += c.isLetterOrNumber() ? c.toLower() : QChar{' '};
  }
  return folded.simplified();
}

std::optional<int> fuzzyScore(const QString &query, const QString &text)
{
  int total = 0;
  for (const QString &word : query.split(' ', Qt::SkipEmptyParts))
  {
    const std::optional<int> score = wordScore(word, text);
    if (!score)
      return std::nullopt;
    total += *score;
  }
  return total;
}
} // namespace dgk
