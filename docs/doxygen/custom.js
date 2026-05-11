(function () {
  function ensureFavicon() {
    var head = document.head || document.getElementsByTagName("head")[0];
    if (!head) return;

    var link = document.querySelector("link[rel='icon']") || document.createElement("link");
    link.rel = "icon";
    link.type = "image/png";
    link.href = "html_title.png";

    if (!link.parentNode) {
      head.appendChild(link);
    }
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", ensureFavicon);
  } else {
    ensureFavicon();
  }
})();
