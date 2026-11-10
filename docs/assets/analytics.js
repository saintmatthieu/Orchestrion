// Counts the download buttons' clicks in Plausible ("Download", with the
// system and the page), on the home page and the play page. A plain click
// waits for the event to be sent before following the link, but never more
// than a second: with Plausible blocked, the event is never sent.
document.addEventListener("click", function (event) {
  var link = event.target.closest("a.dl");
  if (!link) return;
  var props = {
    os: { "dl-win": "Windows", "dl-mac": "macOS", "dl-linux": "Linux" }[link.id] || link.id,
    page: /\/play\/$/.test(location.pathname) ? "play" : "home"
  };
  var elsewhere = event.button !== 0 || event.ctrlKey || event.metaKey ||
    event.shiftKey || event.altKey || (link.target && link.target !== "_self");
  if (elsewhere) {
    window.plausible("Download", { props: props });
    return;
  }
  event.preventDefault();
  var gone = false;
  function go() {
    if (!gone) {
      gone = true;
      location.href = link.href;
    }
  }
  window.plausible("Download", { props: props, callback: go });
  setTimeout(go, 1000);
});
