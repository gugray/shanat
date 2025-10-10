const userNameKey = "userName";
const defaultName = "shady artist";

export function getUserName() {
  const storedName = localStorage.getItem(userNameKey);
  return storedName ?? defaultName;
}

export function setUserName(name) {
  const trimmed = name.trim();
  if (trimmed == "") return;
  localStorage.setItem(userNameKey, trimmed);
}
