import { mkdir, readdir, readFile, rename, stat, writeFile } from "fs/promises";
import { join } from "path";
import { Mutex } from 'async-mutex';

const defaultStorageDir = "../data";
const storageDirEnvVar = "STORAGE_DIR";

let storageDir = defaultStorageDir;
if (process.env[storageDirEnvVar]) {
  storageDir = process.env[storageDirEnvVar];
}

export const mutex = new Mutex();
export const sketchInfos = await loadSketchInfos();

async function loadSketchInfos() {
  const res = {};
  const entries = await readdir(storageDir, { withFileTypes: true });
  for (const entry of entries) {
    if (!entry.isDirectory()) continue;

    const titlePath = join(storageDir, entry.name, "current.title");
    const authorPath = join(storageDir, entry.name, "current.author");

    const title = await readFile(titlePath, "utf-8");
    const author = await readFile(authorPath, "utf-8");
    const stats = await stat(join(storageDir, entry.name));
    const changedAt = stats.mtime;

    res[entry.name] = {
      title: title,
      author: author,
      changedAt: changedAt
    };
  }

  return res;
}

export async function getSketch(name) {

  const fragPath = join(storageDir, name, "current.frag");
  const hydraPath = join(storageDir, name, "current.hydra");

  try {
    return {
      name: name,
      title: sketchInfos[name].title,
      frag: await readFile(fragPath, "utf-8"),
      hydra: await readFile(hydraPath, "utf-8"),
    };
  }
  catch { return null; }
}

export async function renameSketch(oldName, newName, newTitle) {

  if (newName in sketchInfos) return `Sketch '${newName}' already exists`;
  if (!(oldName in sketchInfos)) return `Skatcg '${oldName}' not found`;

  const sketchInfo = sketchInfos[oldName];

  const oldDir = join(storageDir, oldName);
  const newDir = join(storageDir, newName);

  try { await rename(oldDir, newDir); }
  catch { return "Failed to rename sketch directory"; }

  try {
    const titlePath = join(storageDir, newName, "current.title");
    await writeFile(titlePath, newTitle, "utf-8");
    sketchInfo.title = newTitle;
  }
  catch {}

  sketchInfos[newName] = sketchInfo;
  delete sketchInfos[oldName];

  return null;
}

export async function saveSketch(sketchData, author) {

  const sketchDir = join(storageDir, sketchData.name);
  const titlePath = join(sketchDir, "current.title");
  const authorPath = join(sketchDir, "current.author");
  const fragPath = join(sketchDir, "current.frag");
  const hydraPath = join(sketchDir, "current.hydra");


  try {
    // Create now if new
    if (!(sketchData.name in sketchInfos)) {
      await mkdir(sketchDir);
      sketchInfos[sketchData.name] = {
        title: sketchData.title,
        author: author,
      };
      await writeFile(titlePath, sketchData.title, "utf-8");
    }

    await writeFile(authorPath, author, "utf-8");
    await writeFile(fragPath, sketchData.frag, "utf-8");
    await writeFile(hydraPath, sketchData.hydra, "utf-8");

    const stats = await stat(authorPath);
    sketchInfos[sketchData.name].changedAt = stats.mtime;
    return null;
  }
  catch (err) {
    return `Failed to save sketch: ${err}`;
  }
}

