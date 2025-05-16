let scene = new THREE.Scene();
let camera = new THREE.PerspectiveCamera(75, 1.5, 0.1, 1000);
let renderer = new THREE.WebGLRenderer({ canvas: document.getElementById('three-canvas') });
renderer.setSize(document.getElementById("render-area").clientWidth, document.getElementById("render-area").clientHeight);

let cube = new THREE.Mesh(new THREE.BoxGeometry(2, 0.2, 2), new THREE.MeshNormalMaterial());
scene.add(cube);
camera.position.z = 5;

function animate() {
  requestAnimationFrame(animate);
  renderer.render(scene, camera);
}
animate();

function updateOrientation(pitch, roll) {
  cube.rotation.z = pitch * Math.PI / 180;
  cube.rotation.x = roll * Math.PI / 180;

  document.getElementById("pitch").textContent = pitch.toFixed(1);
  document.getElementById("roll").textContent = roll.toFixed(1);

  const time = new Date().toLocaleTimeString();
  let row = document.createElement("tr");
  row.innerHTML = `<td>${time}</td><td>${pitch.toFixed(1)}</td><td>${roll.toFixed(1)}</td>`;
  let logBody = document.getElementById("log-body");
  logBody.appendChild(row);
  if (logBody.rows.length > 15) logBody.removeChild(logBody.firstChild);
}

function fetchData() {
  fetch("/data").then(res => res.json()).then(data => {
    updateOrientation(data.pitch, data.roll);
  });
}
setInterval(fetchData, 100);

function updatePWM() {
  let m1 = document.getElementById("m1").value;
  let m2 = document.getElementById("m2").value;
  let m3 = document.getElementById("m3").value;
  let m4 = document.getElementById("m4").value;

  document.getElementById("label1").textContent = Math.round(m1 / 255 * 100) + "%";
  document.getElementById("label2").textContent = Math.round(m2 / 255 * 100) + "%";
  document.getElementById("label3").textContent = Math.round(m3 / 255 * 100) + "%";
  document.getElementById("label4").textContent = Math.round(m4 / 255 * 100) + "%";

  fetch(`/setPWM?m1=${m1}&m2=${m2}&m3=${m3}&m4=${m4}`);
}

function syncAll(val) {
  ['m1', 'm2', 'm3', 'm4'].forEach((id, i) => {
    document.getElementById(id).value = val;
    document.getElementById("label" + (i + 1)).textContent = Math.round(val / 255 * 100) + "%";
  });
  document.getElementById("master").value = val;
  document.getElementById("labelMaster").textContent = Math.round(val / 255 * 100) + "%";
  updatePWM();
}

function recalibrate() {
  const btn = document.getElementById("recalibrateBtn");
  btn.disabled = true;
  const originalText = btn.textContent;
  btn.textContent = "Calibrating...";

  fetch("/recalibrate").then(() => {
    btn.disabled = false;
    btn.textContent = originalText;
  });
}


function emergencyStop() {
  ['m1', 'm2', 'm3', 'm4'].forEach((id, i) => {
    document.getElementById(id).value = 0;
    document.getElementById("label" + (i + 1)).textContent = "0%";
  });
  document.getElementById("master").value = 0;
  document.getElementById("labelMaster").textContent = "0%";
  fetch("/setPWM?m1=0&m2=0&m3=0&m4=0");
}
