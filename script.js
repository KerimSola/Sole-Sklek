// ---------- Firebase konfiguracija ----------
const firebaseConfig = {
  apiKey: "AIzaSyACCRX9SDyw9T417CEEYjK2gotLPc1K_FA",
  authDomain: "sole-sklek-iot.firebaseapp.com",
  databaseURL: "https://sole-sklek-iot-default-rtdb.europe-west1.firebasedatabase.app",
  projectId: "sole-sklek-iot",
  storageBucket: "sole-sklek-iot.firebaseapp.com",
  messagingSenderId: "303593985687",
  appId: "1:303593985687:web:4998821e197d609727bbd2",
  measurementId: "G-XKXL1DQDF3"
};

// Inicijalizacija Firebase-a
firebase.initializeApp(firebaseConfig);
const db = firebase.database();
let neKliknuto = false;

document.getElementById('Tajmer1').addEventListener('click', () => {
    neKliknuto = true;
    console.log("Korisnik je kliknuo NE → tajmer neće biti pokrenut");
});

// ---------- Reference na Firebase čvorove ----------
const gornjaRef = db.ref('GornjaGranica');
const komandaRef = db.ref('komandaMeasure');
const startRef = db.ref('komandaStart');
const brojRef = db.ref('BrojSklekova');

// ---------- Dugme "Izmjeri duzinu" ----------
document.getElementById('IzmjeriDuzinu').addEventListener('click', () => {
  komandaRef.set(1).then(() => console.log("Signal za mjerenje poslan ESP-u"));

  // Ažuriraj odmah kad ESP upiše vrijednost
  gornjaRef.on('value', snapshot => {
    const vrijednost = snapshot.val() || 0;
    document.getElementById('Duzina').textContent = vrijednost;
    console.log("Gornja granica ažurirana:", vrijednost);
  });
});

// ---------- Dugme "Start" ----------
document.getElementById('Start').addEventListener('click', () => {
  startRef.set(1);
  console.log("START poslan ESP-u");

  if (neKliknuto) {
        console.log("Tajmer nije pokrenut jer je korisnik prethodno kliknuo NE");
        return; // izlaz iz funkcije
    }
  // Dohvati vrijednost iz inputa
let vrijeme = parseInt(document.getElementById('unosMinuta').value, 10);

    if (isNaN(vrijeme) || vrijeme <= 0) {
        alert("Unesi ispravan broj sekundi!");
        return;
    }

    // Ako interval već postoji, očisti ga
    if (timerInterval) clearInterval(timerInterval);

    // Prikaz početnog vremena odmah
    document.getElementById('tajmerLabel').textContent = formatTime(vrijeme);

    // Pokreni odbrojavanje
    timerInterval = setInterval(() => {
        vrijeme--;

        if (vrijeme >= 0) {
            document.getElementById('tajmerLabel').textContent = formatTime(vrijeme);
        } else {
          startRef.set(0);
            clearInterval(timerInterval);
            alert("Tajmer završen!");
        }
    }, 1000);
});


// Funkcija za format hh:mm:ss
let timerInterval = null; // globalni interval

// Funkcija za formatiranje vremena u hh:mm:ss
function formatTime(seconds) {
    const h = Math.floor(seconds / 3600).toString().padStart(2, '0');
    const m = Math.floor((seconds % 3600) / 60).toString().padStart(2, '0');
    const s = (seconds % 60).toString().padStart(2, '0');
    return `${h}:${m}:${s}`;
}

function getValue() {
    // Dohvati input element
    const input = document.getElementById('unosMinuta');

    // 1. Opcija: parseInt
    const intValue1 = parseInt(input.value, 10);
    console.log(intValue1);
    return intValue1;
}



// ---------- Dugme "Reset" ----------
document.getElementById('Reset').addEventListener('click', () => {
  startRef.set(0);              // Zaustavi mjerenje na ESP-u
  brojRef.set(0);               // Resetuj brojač u Firebase-u
  console.log("RESET poslan ESP-u");

  neKliknuto = false;
  if (timerInterval) clearInterval(timerInterval);
  document.getElementById('tajmerLabel').textContent = "00:00:00";
  document.getElementById('brojLabel').textContent = "0";
});

// ---------- Real-time prikaz broja sklekova ----------
brojRef.on('value', snapshot => {
  const broj = snapshot.val() || 0;
  document.getElementById('brojLabel').textContent = broj;
  console.log("Broj sklekova:", broj);
});

// ---------- Real-time prikaz gornje granice (opcionalno) ----------
gornjaRef.on('value', snapshot => {
  const vrijednost = snapshot.val();
  document.getElementById('Duzina').textContent = vrijednost;
});

// ---------- Status indikacija (opcionalno) ----------
startRef.on('value', snapshot => {
  const stanje = snapshot.val();
  if (stanje === 1) {
    console.log("STATUS: START aktivan!");
  } else {
    console.log("STATUS: Mjerenje zaustavljeno.");
  }
});

