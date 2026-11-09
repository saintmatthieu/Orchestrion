// Opens a score of Rolls in Orchestrion: /play/?id=<id>. Browsers can't tell
// whether anything handles a link, so what follows it is a guess: the words
// on the page must hold true if the app did open after all.
(function () {
  var page = document.documentElement.dataset;
  var params = new URLSearchParams(location.search);
  var id = params.get("id") || "";
  var valid = id.length <= 128 && /^[a-z0-9]+(-[a-z0-9]+)*$/.test(id);

  function show(elementId) { document.getElementById(elementId).hidden = false; }
  function element(elementId) { return document.getElementById(elementId); }

  if (!valid) {
    element("heading").textContent = page.invalid;
    show("browse");
    return;
  }

  var scorePage = "https://rolls.orchestrion.app/s/" + id + "/";
  element("back").href = scorePage;
  show("back-row");

  fetch("https://rolls.orchestrion.app/index.json")
    .then(function (r) { return r.ok ? r.json() : Promise.reject(); })
    .then(function (index) {
      var score = (index.scores || []).find(function (s) { return s.id === id; });
      if (!score || !score.title) return;
      element("heading").textContent = page.opening.replace("%1", score.title);
      element("library-title").textContent = score.title;
    })
    .catch(function () { /* the page says "this score" */ });

  var mobile = /Android|iPhone|iPad|iPod|Mobi/.test(navigator.userAgent) ||
    (/Mac/.test(navigator.userAgent) && navigator.maxTouchPoints > 1);
  if (mobile) {
    element("heading").textContent = page.mobile;
    return;
  }

  var left = false, timer = null;
  function offer() {
    show("fallback");
    loadDownloads();
  }
  function open() {
    left = false;
    clearTimeout(timer);
    location.href = "orchestrion://rolls/" + id;
    timer = setTimeout(function () { if (!left) offer(); }, 1500);
  }
  function leave() { left = true; clearTimeout(timer); }
  // Back from the app, or from a browser's prompt answered with "no".
  function back() { if (left) offer(); }

  window.addEventListener("blur", leave);
  window.addEventListener("focus", back);
  document.addEventListener("visibilitychange", function () {
    if (document.hidden) leave(); else back();
  });
  element("again").addEventListener("click", open);
  open();

  var loaded = false;
  function loadDownloads() {
    if (loaded) return;
    loaded = true;
    var culture = page.culture;
    fetch("https://api.github.com/repos/saintmatthieu/Orchestrion/releases/latest")
      .then(function (r) { return r.ok ? r.json() : Promise.reject(); })
      .then(function (release) {
        function link(elementId, suffix) {
          var asset = (release.assets || []).find(function (a) { return a.name.endsWith(suffix); });
          if (asset) element(elementId).href = asset.browser_download_url;
        }
        link("dl-win", "-Windows-" + culture + ".msi");
        link("dl-mac", "-macOS.dmg");
        link("dl-linux", "-Linux-x86_64.AppImage");
      })
      .catch(function () { /* keep the releases page */ });
    var ua = navigator.userAgent;
    var platform = ua.indexOf("Win") !== -1 ? "dl-win"
                 : ua.indexOf("Mac") !== -1 ? "dl-mac"
                 : ua.indexOf("Linux") !== -1 ? "dl-linux" : null;
    if (platform) element(platform).classList.add("primary");
  }
})();
