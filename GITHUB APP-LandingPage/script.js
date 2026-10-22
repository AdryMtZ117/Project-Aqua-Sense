const reveals = document.querySelectorAll('.reveal');
  const observer = new IntersectionObserver((entries) => {
    entries.forEach((e, i) => {
      if (e.isIntersecting) {
        setTimeout(() => e.target.classList.add('visible'), i * 80);
        observer.unobserve(e.target);
      }
    });
  }, { threshold: 0.1 });
  reveals.forEach(el => observer.observe(el));
 
  function animateCount(el, target, suffix = '') {
    let start = 0; const dur = 1800; const step = 16;
    const inc = target / (dur / step);
    const timer = setInterval(() => {
      start = Math.min(start + inc, target);
      el.textContent = Math.round(start) + suffix;
      if (start >= target) clearInterval(timer);
    }, step);
  }
 
  const statsObserver = new IntersectionObserver((entries) => {
    entries.forEach(e => {
      if (e.isIntersecting) {
        const nums = e.target.querySelectorAll('.stat-number');
        nums.forEach((el, i) => {
          if (i === 1) { el.textContent = '24/7'; return; }
          if (i === 3) { el.textContent = '6 años'; return; }
          const targets = [98, 0, 5, 0];
          const suffixes = ['%', '', '', ''];
          animateCount(el, targets[i], suffixes[i]);
        });
        statsObserver.unobserve(e.target);
      }
    });
  }, { threshold: 0.5 });
 
  const strip = document.querySelector('.stats-strip');
  if (strip) statsObserver.observe(strip);