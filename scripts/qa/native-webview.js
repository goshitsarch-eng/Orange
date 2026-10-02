// Runs inside the real Dioxus Desktop WebView via the opt-in ui-qa feature.
// Seed an isolated collection with two local tracks before running this suite.
const checks = [];
const delay = ms => new Promise(resolve => setTimeout(resolve, ms));
const assert = (value, label) => { if (!value) throw new Error(label); checks.push(label); };
const wait = async predicate => {
  for (let i = 0; i < 100; ++i) { if (predicate()) return; await delay(50); }
  throw new Error('Timed out waiting for UI state');
};
const button = label => [...document.querySelectorAll('button')].find(node => node.textContent.trim() === label);
const click = async node => { if (!node) throw new Error('Missing control'); node.click(); await delay(250); };
const set = async (selector, value, event = 'input') => {
  const node = document.querySelector(selector); if (!node) throw new Error(selector);
  node.value = value; node.dispatchEvent(new Event(event, {bubbles:true})); await delay(250);
};
const closeDialog = async () => {
  document.querySelector('.dialog').dispatchEvent(new KeyboardEvent('keydown', {key:'Escape',bubbles:true}));
  await wait(() => !document.querySelector('.dialog'));
};
try {
  await wait(() => document.querySelectorAll('tbody tr').length === 2);
  assert(document.querySelector('#play-pause').disabled, 'Empty queue disables play');
  await set('#search', 'no-such-track');
  assert(document.querySelectorAll('tbody tr').length === 0, 'Search filters collection');
  await set('#search', '');
  await click(document.querySelector('tbody tr button'));
  await click(document.querySelectorAll('tbody tr button')[1]);
  await click(document.querySelector('#nav-queue'));
  await wait(() => document.querySelectorAll('tbody tr').length === 2);
  await set('#repeat', '2', 'change');
  await click(document.querySelector('tbody tr button'));
  await wait(() => document.querySelector('#play-pause').getAttribute('aria-label') === 'Pause');
  assert(true, 'Queue starts real playback pipeline');
  await click(document.querySelector('#play-pause'));
  await wait(() => document.querySelector('#play-pause').getAttribute('aria-label') === 'Play');
  await set('#volume', '37');
  await click(document.querySelector('#next'));
  await click(document.querySelector('#previous'));
  await click(document.querySelector('#stop'));
  await click(document.querySelector('#save-playlist'));
  await set('#playlist-name', 'Música 日本 QA');
  await click(document.querySelector('#playlist-save'));
  await wait(() => [...document.querySelectorAll('.sidebar button')].some(n => n.textContent === 'Música 日本 QA'));
  assert(true, 'Unicode playlist saved and listed');
  await click(document.querySelector('#clear-queue'));
  await click(button('Undo'));
  await wait(() => document.querySelectorAll('tbody tr').length === 2);
  await click(button('Redo'));
  await wait(() => document.querySelectorAll('tbody tr').length === 0);
  await click(document.querySelector('#nav-playlists'));
  assert(document.querySelector('h1').textContent === 'Playlists', 'Saved playlists have their own page');
  await click(button('Load'));
  assert(document.querySelector('#repeat').value === '2', 'Repeat selection survives page remount');
  await wait(() => document.querySelectorAll('tbody tr').length === 2);
  await click(document.querySelector('#nav-radio'));
  await set('#station-name', '日本 Jazz');
  await set('#station-url', 'file:///etc/passwd');
  await click(document.querySelector('#save-station'));
  await wait(() => document.querySelector('[role=alert]'));
  assert(true, 'Invalid radio URL produces visible error');
  await click(button('Dismiss'));
  await set('#station-url', 'https://example.org/jazz');
  await click(document.querySelector('#save-station'));
  await wait(() => document.querySelector('.station-row'));
  assert(document.querySelector('.station-row').textContent.includes('日本 Jazz'), 'Custom station appears');
  for (const page of ['files','devices','settings']) {
    await click(document.querySelector('#nav-' + page));
    assert(!!document.querySelector('h1'), page + ' navigation renders content');
    if(page==='files') {
      await wait(() => document.querySelector('#file-entries').textContent.includes('Artist'));
      await click([...document.querySelectorAll('#file-entries button')].find(n=>n.textContent.includes('Artist')));
      await wait(() => document.querySelector('#file-entries').textContent.includes('Album'));
      await click([...document.querySelectorAll('#file-entries button')].find(n=>n.textContent.includes('Album')));
      await wait(() => document.querySelectorAll('#file-entries tr').length===2);
      assert(document.querySelector('#files-path').textContent.includes('Música 日本'), 'Filesystem browser navigates real Unicode folders');
      await click(document.querySelector('#files-up'));
      await wait(() => document.querySelector('#file-entries').textContent.includes('Album'));
    }
  }
  await set('#theme', 'dark', 'change');
  await wait(() => document.querySelector('.app').dataset.theme === 'dark');
  assert(true, 'Dark theme updates from backend setting');
  await set('#theme', 'light', 'change');
  await wait(() => document.querySelector('.app').dataset.theme === 'light');
  assert(true, 'Light theme updates from backend setting');
  await set('#theme', 'system', 'change');
  await wait(() => document.querySelector('.app').dataset.theme === 'system');
  assert(true, 'System theme updates from backend setting');
  await set('#eq-0', '3', 'change');
  await set('#theme', 'dark', 'change');
  document.querySelector('.app-menu summary').click();
  await click(button('About Orange'));
  assert(document.querySelector('.dialog').textContent.includes('Made by Gosh'), 'About preserves identity');
  await closeDialog();
  await click(button('Lyrics'));
  assert(document.querySelector('.dialog').textContent.includes('Lyrics'), 'Lyrics dialog renders');
  await closeDialog();
  document.querySelector('.app-menu').open = false;
  await click(document.querySelector('#nav-collection'));
  const row=document.querySelector('tbody tr');
  row.dispatchEvent(new MouseEvent('contextmenu',{bubbles:true}));
  await wait(() => document.querySelector('.dialog'));
  await set('[aria-label="Track rating"]', '0.6', 'change');
  await closeDialog();
  await click(document.querySelector('#rescan'));
  await wait(() => document.querySelector('.status').textContent.includes('Rescanned'));
  assert(document.querySelectorAll('tbody tr').length === 2, 'Background rescan retains collection');
  assert(!document.querySelector('[role=alert]'), 'No unresolved application errors');
  document.querySelector('tbody tr').dispatchEvent(new MouseEvent('contextmenu',{bubbles:true}));
  await wait(() => document.querySelector('.dialog'));
  assert(document.querySelector('[aria-label="Track rating"]').value === '0.6', 'Saved fractional rating is selected when its dialog opens');
  await click(button('Edit Tags…'));
  const titleInput=document.querySelector('.tag-fields input');
  titleInput.value='日本 QA Track';titleInput.dispatchEvent(new Event('input',{bubbles:true}));
  await delay(250);
  await click(button('Save Tags'));
  await wait(() => document.querySelector('.status').textContent.includes('Tags saved'));
  await click(document.querySelector('#rescan'));
  await wait(() => document.querySelector('tbody').textContent.includes('日本 QA Track'));
  assert(!document.querySelector('[role=alert]'), 'Rendered tag editor writes real Unicode audio metadata');
  document.querySelector('tbody tr').dispatchEvent(new MouseEvent('contextmenu',{bubbles:true}));
  await wait(() => document.querySelector('.dialog'));
  await click(button('Convert Audio…'));
  assert(!!button('Choose Output File…'), 'Audio conversion dialog exposes native output selection');
  await closeDialog();
  await click(document.querySelector('#nav-playlists'));
  await click(button('Delete'));
  await wait(() => !document.querySelector('.sidebar').textContent.includes('Música 日本 QA'));
  assert(true, 'Saved playlist deletion updates persistent library');

  for (const icon of document.querySelectorAll('button.icon')) {
    assert(!!icon.getAttribute('aria-label') && !!icon.title, 'Transport control has accessible name and tooltip');
  }
  return {ok:true,checks};
} catch(error) {return {ok:false,error:String(error),checks};}
