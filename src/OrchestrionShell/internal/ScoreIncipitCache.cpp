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
#include "ScoreIncipitCache.h"
#include "ScoreTitles.h"

#include <draw/painter.h>
#include <engraving/dom/bracket.h>
#include <engraving/dom/masterscore.h>
#include <engraving/dom/measurebase.h>
#include <engraving/dom/mscore.h>
#include <engraving/dom/page.h>
#include <engraving/dom/system.h>
#include <notation/imasternotation.h>
#include <project/inotationproject.h>

#include "log.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

namespace dgk
{
namespace
{
/**
 * Bumped whenever the rendering changes, so that the entries rendered before
 * are rendered again.
 */
constexpr int renderVersion = 2;

/**
 * How big a staff space is drawn, in logical pixels: a staff is four of
 * them high. Every score is drawn at that size, so that the cards' music is
 * all of a piece; only a score whose first system would not fit the strip
 * (three staves and more) is drawn smaller.
 */
constexpr double spatiumPixels = 6.0;

/**
 * How far above and below its staves a system's band may reach for what
 * sticks out (ledger-line notes, dynamics, tempo marks), in staff spaces.
 */
constexpr double minOverhang = 1.5;
constexpr double maxOverhang = 6.0;

/**
 * Loads the score for drawing only: no playback, and none of the import's
 * questions (a MusicXML file with errors asks whether to go on; the answer
 * here is always yes, whatever the shelf shows of it).
 */
mu::project::INotationProjectPtr
loadQuietly(const mu::project::IProjectCreator &creator, const QString &path)
{
  const mu::project::INotationProjectPtr project =
      creator.newProject(dgk::iocContext());
  mu::project::OpenParams params;
  params.disablePlayback = true;
  params.forcePageMode = true;
  const bool wasNoGui = mu::engraving::MScore::noGui;
  mu::engraving::MScore::noGui = true;
  const muse::Ret ret = project->load(muse::io::path_t{path}, params);
  mu::engraving::MScore::noGui = wasNoGui;
  if (!ret)
  {
    LOGW() << "Could not load " << path << ": " << ret.toString();
    return nullptr;
  }
  return project;
}

/**
 * Where the strip starts: at the brackets that open the system, or its first
 * measure if it has none — past the instrument names, which would take much
 * of the strip and say little.
 */
double incipitLeft(const mu::engraving::System &system, double spatium)
{
  double left = system.measures().front()->pageBoundingRect().left();
  for (const mu::engraving::Bracket *bracket : system.brackets())
    left = std::min(left, bracket->pageBoundingRect().left());
  return left - 0.5 * spatium;
}

/** The first system that holds music, i.e. isn't a title frame. */
const mu::engraving::System *
firstMusicSystem(const mu::engraving::MasterScore &score)
{
  const auto &systems = score.systems();
  const auto it = std::find_if(systems.begin(), systems.end(),
                               [](const mu::engraving::System *system) {
                                 return !system->vbox() && system->page() &&
                                        !system->measures().empty();
                               });
  return it == systems.end() ? nullptr : *it;
}

QImage drawIncipit(const mu::engraving::rendering::IScoreRenderer &renderer,
                   mu::engraving::MasterScore &score)
{
  using namespace mu::engraving;

  const System *const system = firstMusicSystem(score);
  if (!system)
    return {};
  const Page *const page = system->page();
  const auto &pages = score.pages();
  const auto pageIt = std::find(pages.begin(), pages.end(), page);
  if (pageIt == pages.end())
    return {};

  // The band the system occupies on its page: its staves, and what sticks
  // out of them within reason.
  const double spatium = score.style().spatium();
  const auto overhang = [&](double margin)
  { return std::clamp(margin, minOverhang * spatium, maxOverhang * spatium); };
  const RectF staves = system->pageBoundingRect();
  const double top = staves.top() - overhang(system->minTop());
  const double bottom = staves.bottom() + overhang(system->minBottom());

  constexpr int imageWidth =
      ScoreIncipitCache::width * ScoreIncipitCache::pixelRatio;
  constexpr int imageHeight =
      ScoreIncipitCache::height * ScoreIncipitCache::pixelRatio;
  // Every score at the same size, unless its staves themselves don't fit:
  // what sticks out of a crowded band is cut rather than everything shrunk.
  const double scale =
      std::min(spatiumPixels * ScoreIncipitCache::pixelRatio / spatium,
               imageHeight / (staves.height() + 2 * minOverhang * spatium));
  const double viewHeight = imageHeight / scale;
  // Centred on the band, as far as the staves stay in view.
  const double highest = staves.top() - minOverhang * spatium;
  const double lowest =
      std::min(staves.bottom() + minOverhang * spatium - viewHeight, highest);
  const double viewTop =
      std::clamp((top + bottom - viewHeight) / 2, lowest, highest);
  const double left = incipitLeft(*system, spatium);
  const RectF band{left, viewTop, imageWidth / scale, viewHeight};

  QImage image{imageWidth, imageHeight, QImage::Format_ARGB32_Premultiplied};
  image.fill(Qt::transparent);
  {
    muse::draw::Painter painter{&image, "incipit"};
    painter.setAntialiasing(true);
    painter.scale(scale, scale);
    painter.translate(-band.left(), -band.top());
    painter.setClipRect(band);
    painter.setClipping(true);

    rendering::IScoreRenderer::ScorePaintOptions opt;
    opt.isPrinting = true;
    opt.isSetViewport = false;
    opt.printPageBackground = false;
    opt.fromPage = opt.toPage = static_cast<int>(pageIt - pages.begin());
    opt.frameRect = band.translated(page->pos());
    renderer.paintScore(&painter, &score, opt);
    painter.endDraw();
  }
  return image;
}
} // namespace

QString ScoreIncipitCache::directory() const
{
  return globalConfiguration()->userAppDataPath().toQString() + "/incipits";
}

QString ScoreIncipitCache::key(const QString &scorePath) const
{
  const QFileInfo info{scorePath};
  const QString identity = QString("%1|%2|%3|%4")
                               .arg(info.absoluteFilePath())
                               .arg(info.size())
                               .arg(info.lastModified().toMSecsSinceEpoch())
                               .arg(renderVersion);
  return QCryptographicHash::hash(identity.toUtf8(), QCryptographicHash::Sha1)
      .toHex();
}

std::optional<ScoreIncipitCache::Entry>
ScoreIncipitCache::cached(const QString &scorePath) const
{
  const QString stem = directory() + "/" + key(scorePath);
  QFile file{stem + ".json"};
  if (!file.open(QIODevice::ReadOnly))
    return std::nullopt;
  const QJsonObject json = QJsonDocument::fromJson(file.readAll()).object();
  Entry entry{json.value("title").toString(), json.value("composer").toString(),
              QString{}};
  if (json.value("image").toBool() && QFileInfo::exists(stem + ".png"))
    entry.imagePath = stem + ".png";
  return entry;
}

ScoreIncipitCache::Entry ScoreIncipitCache::render(const QString &scorePath)
{
  const muse::io::path_t path{scorePath};
  Entry entry{titleFromFileName(path), QString{}, QString{}};
  QImage image;
  if (const auto project = loadQuietly(*projectCreator(), scorePath))
  {
    const mu::project::ProjectMeta meta = project->metaInfo();
    entry.title = scoreTitle(meta, path);
    entry.composer = scoreComposer(meta);
    if (mu::engraving::MasterScore *const score =
            project->masterNotation()->masterScore())
      image = drawIncipit(*scoreRenderer(), *score);
  }

  QDir{}.mkpath(directory());
  const QString stem = directory() + "/" + key(scorePath);
  if (!image.isNull())
  {
    if (image.save(stem + ".png"))
      entry.imagePath = stem + ".png";
    else
      LOGE() << "Could not save " << stem << ".png";
  }
  QFile file{stem + ".json"};
  if (file.open(QIODevice::WriteOnly))
    file.write(QJsonDocument{
        QJsonObject{
            {"title", entry.title},
            {"composer", entry.composer},
            {"image", !entry.imagePath.isEmpty()},
        }}.toJson());
  return entry;
}
} // namespace dgk
