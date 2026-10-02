// Launch the native app with an M3U containing two real local WAV tracks.
const delay = ms => new Promise(resolve => setTimeout(resolve, ms));
const checks = [];
const assert = (value, label) => {
  if (!value) throw new Error(label);
  checks.push(label);
};
try {
  document.querySelector('#nav-queue').click();
  for (let i = 0; i < 100 && (
    document.querySelector('h1')?.textContent !== 'Play Queue' ||
    document.querySelectorAll('tbody tr').length !== 2
  ); ++i) {
    await delay(50);
  }
  const rows = [...document.querySelectorAll('tbody tr')];
  assert(rows.length === 2, 'Playlist launch expands into two queue tracks');
  assert(rows[0].textContent.includes('Launch 日本 First'), 'Playlist title survives native launch');
  assert(rows[1].textContent.includes('02 Track 2'), 'Untitled entry uses its Unicode-safe file stem');
  assert(!document.querySelector('[role=alert]'), 'Playlist argument produces no audio or parser error');
  return {ok:true, checks};
} catch (error) {
  return {ok:false, checks, error:String(error)};
}
