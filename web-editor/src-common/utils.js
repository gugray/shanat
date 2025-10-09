export function truncate(str, num) {
  if (str.length > num) {
    return str.slice(0, num) + "...";
  } else {
    return str;
  }
}

export function esc(str, quotes) {
  str = str.replaceAll("&", "&amp;");
  str = str.replaceAll("<", "&lt;");
  str = str.replaceAll(">", "&gt;");
  if (quotes) {
    str = str.replaceAll("'", "&apos;");
    str = str.replaceAll('"', "&quot;");
  }
  return str;
}
