// The play page (play/?id=<id>): opens a score of Rolls in Orchestrion
// through its link, orchestrion://rolls/<id>, which the app registers.
//
// A browser can't tell whether an application handles a link. If the page
// is still in front after a moment, the app most likely didn't open: the
// page offers it ("fallback"). Its wording holds even if the app did open
// (the visitor may have taken a while to answer the browser's prompt).
//
// The texts are in the page, one per language; this script only picks the
// panels (see play.css) and fills in the score's title and links.
(function () {
  var ROLLS = "https://rolls.orchestrion.app/";
  var root = document.documentElement;
  var params = new URLSearchParams(location.search);
  var id = params.get("id") || "";

  // Where the visitor comes from: the Rolls links carry utm_* parameters.
  var visit = {
    id: id,
    lang: root.lang,
    source: params.get("utm_source") || "",
    medium: params.get("utm_medium") || ""
  };

  // Counts what visitors do here: which scores and pages lead to
  // Orchestrion. No counter is installed yet; this is where it is called.
  function track(event, extra) {
    var props = {};
    for (var k in visit) props[k] = visit[k];
    for (var e in extra || {}) props[e] = extra[e];
  }

  function each(selector, f) {
    Array.prototype.forEach.call(document.querySelectorAll(selector), f);
  }

  // The language links keep the score.
  each("a.lang", function (a) {
    a.href = a.getAttribute("href") + location.search;
    a.addEventListener("click", function () {
      try { localStorage.setItem("lang", a.getAttribute("hreflang")); } catch (e) {}
    });
  });

  // Only a score's id: never an address to send the app to.
  if (!/^[a-z0-9]+(-[a-z0-9]+)*$/.test(id)) {
    root.classList.add("broken");
    track("broken-link");
    return;
  }

  var link = "orchestrion://rolls/" + id;
  each("a.open-link", function (a) { a.href = link; });
  each("a.rolls-page", function (a) { a.href = ROLLS + "s/" + id + "/"; });

  fetch(ROLLS + "index.json")
    .then(function (r) { return r.ok ? r.json() : Promise.reject(); })
    .then(function (index) {
      var score = (index.scores || []).find(function (s) { return s.id === id; });
      if (!score) return;
      var quote = { fr: ["« ", " »"], de: ["„", "“"] }[root.lang] || ["“", "”"];
      each(".score-title", function (span) {
        span.textContent = quote[0] + score.title + quote[1];
      });
      if (score.page) each("a.rolls-page", function (a) { a.href = ROLLS + score.page; });
    })
    .catch(function () { /* keep "the score" */ });

  track("view");

  if (root.classList.contains("mobile")) {
    track("mobile");
    var send = document.getElementById("send-link");
    send.addEventListener("click", function () {
      var url = location.href, title = document.title;
      function mail() {
        location.href = "mailto:?subject=" + encodeURIComponent(title) +
                        "&body=" + encodeURIComponent(url);
      }
      if (navigator.share) {
        navigator.share({ title: title, url: url }).catch(function () {});
      } else if (navigator.clipboard) {
        navigator.clipboard.writeText(url).then(function () {
          var label = send.textContent;
          send.textContent = send.getAttribute("data-copied");
          setTimeout(function () { send.textContent = label; }, 2000);
        }, mail);
      } else {
        mail();
      }
    });
    return;
  }

  function showFallback() {
    if (root.classList.contains("fallback")) return;
    root.classList.add("fallback");
    track("fallback");
  }

  // The app (or the browser's prompt) takes the focus when the link works.
  var left = false;
  window.addEventListener("blur", function () { left = true; });
  document.addEventListener("visibilitychange", function () {
    if (document.hidden) left = true;
  });
  // Back here afterwards: the visitor may need the offer after all.
  window.addEventListener("focus", function () { if (left) showFallback(); });

  // Once the page is loaded: leaving for the link may stop what still loads.
  function launch() {
    location.href = link;
    setTimeout(function () { if (!left) showFallback(); }, 1500);
  }
  if (document.readyState === "complete") launch();
  else window.addEventListener("load", launch);

  each("a.open-link", function (a) {
    a.addEventListener("click", function () { track("try-again"); });
  });

  // ---- Downloads, as on the home page --------------------------------------
  var culture = { fr: "fr-FR", de: "de-DE" }[root.lang] || "en-US";
  fetch("https://api.github.com/repos/saintmatthieu/Orchestrion/releases/latest")
    .then(function (r) { return r.ok ? r.json() : Promise.reject(); })
    .then(function (release) {
      function direct(elementId, suffix) {
        var asset = (release.assets || []).find(function (a) {
          return a.name.endsWith(suffix);
        });
        if (asset) document.getElementById(elementId).href = asset.browser_download_url;
      }
      direct("dl-win", "-Windows-" + culture + ".msi");
      direct("dl-mac", "-macOS.dmg");
      direct("dl-linux", "-Linux-x86_64.AppImage");
    })
    .catch(function () { /* keep the releases page */ });

  var ua = navigator.userAgent;
  var mine = ua.indexOf("Win") !== -1 ? "dl-win"
           : ua.indexOf("Mac") !== -1 ? "dl-mac"
           : ua.indexOf("Linux") !== -1 ? "dl-linux" : null;
  if (mine) document.getElementById(mine).classList.add("primary");
  each(".downloads a.dl", function (a) {
    a.addEventListener("click", function () {
      track("download", { platform: a.id.replace("dl-", "") });
    });
  });
})();
