// ===========================
// POWER STATUS LOGIC
// ===========================

(() => {
  const STATUS_URL = '/api/power-status.json';

  const setBtn = (btns, i, state) => {
    const b = btns[i];
    if (!b) return;
    b.textContent = state ? 'ON' : 'OFF';
    b.classList.remove('buttonWarn', 'buttonOn', 'buttonOff');
    b.classList.add(state ? 'buttonOn' : 'buttonOff');
    b.setAttribute('aria-pressed', String(state));
  };

  fetch(STATUS_URL, {
      cache: 'no-store',
      keepalive: false
    })
    .then(r => r.json())
    .then(data => {
      const N = Number(data.devicesstatus) || 0;
      const btns = Array.from({
        length: N
      }, (_, i) => document.getElementById('button' + i));

      for (let i = 0; i < N; i++) {
        setBtn(btns, i, !!data['dev_power_' + i]);
      }

      btns.forEach((b, i) => {
        if (!b) return;
        b.addEventListener('click', () => {
          fetch(STATUS_URL, {
            method: 'POST',
            headers: {
              'Content-Type': 'application/json'
            },
            body: JSON.stringify({
              ['dev_power_' + i]: true
            })
          }).then(() => {
            setTimeout(() => poll(), 300); // ⏱️ 300ms delay
          });
        });
      });

      let inFlight = false;

      const poll = () => {
        if (inFlight) return;
        inFlight = true;

        fetch(STATUS_URL, {
            cache: 'no-store',
            keepalive: false
          })
          .then(r => r.json())
          .then(d => {
            for (let i = 0; i < N; i++) setBtn(btns, i, !!d['dev_power_' + i]);
          })
          .finally(() => (inFlight = false));
      };

      const intervalId = setInterval(poll, 1000);
      window.addEventListener('beforeunload', () => clearInterval(intervalId));
    });
})();


// ===========================
// INITIAL CONFIG LOADING
// ===========================

(() => {
  fetch('/api/power-config.json', {
      cache: 'no-store',
      keepalive: false
    })
    .then(r => r.json())
    .then(data => {
      const N = Number(data.devicesconfig) || 0;
      const namesArray = Array.isArray(data.names) ? data.names : [];

      for (let i = 0; i < N; i++) {
        const el = document.getElementById('devicename' + i);
        if (el) el.value = namesArray[i] ?? '';
      }
    });
})();


// ===========================
// CONFIG SAVE HANDLING
// ===========================

(() => {
  const CONFIG_URL = '/api/power-config.json';
  const form = document.getElementById('nameForm');
  const saveBtn = document.querySelector('button[form="nameForm"][type="submit"]');

  if (!form) return;

  form.addEventListener('submit', e => {
    e.preventDefault();

    if (typeof form.reportValidity === 'function' && !form.reportValidity()) {
      return;
    }

    const names = [];
    let i = 0;
    while (true) {
      const el = document.getElementById('devicename' + i);
      if (!el) break;
      names.push(el.value.trim());
      i++;
    }

    const payload = {
      devicesconfig: i,
      names
    };

    fetch(CONFIG_URL, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify(payload),
        cache: 'no-store',
        keepalive: false
      })
      .then(r => {
        if (!r.ok) throw new Error('Save failed');
        if (saveBtn) {
          const t = saveBtn.textContent;
          saveBtn.textContent = 'Saved';
          setTimeout(() => (saveBtn.textContent = t), 1200);
        }
      })
      .catch(() => {
        if (saveBtn) {
          const t = saveBtn.textContent;
          saveBtn.textContent = 'Retry';
          setTimeout(() => (saveBtn.textContent = t), 1200);
        }
      });
  });
})();