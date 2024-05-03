

async function fetchData() {
    try {
        const response = await fetch('https://superdomain.asuscomm.com/nodered/device/' + document.querySelector("#deviceDropdown").value);
        const data = await response.json();
        console.log(data);
        return data;
    } catch (error) {
        console.error('Error fetching data:', error);
        return [];
    }
}

async function fetchDeviceData() {
    try {
        const response = await fetch('https://superdomain.asuscomm.com/nodered/getDeviceData');
        const data = await response.json();
        return data;
    } catch (error) {
        console.error('Error fetching data:', error);
        return [];
    }
}

let markers = [];
let selectedRowIds = [];

async function displayData() {
    console.log("Calling fetchData to retrieve data");
    const data = await fetchData();
    insertDataToGrid(data);

}

function formatDate(date) {
    const options = { year: 'numeric', month: '2-digit', day: '2-digit', hour: '2-digit', minute: '2-digit', second: '2-digit', hour12: false };
    return new Intl.DateTimeFormat('en-US', options).format(date);
}

function insertDataToGrid(data) {
    const tableBody = document.getElementById('dataGrid').getElementsByTagName('tbody')[0];
    tableBody.innerHTML = '';

    data.slice(0, 1000).forEach((rowData, index) => {
        let newRow = tableBody.insertRow();
        newRow.setAttribute('data-row-id', index);

        Object.entries(rowData).forEach(([key, value]) => {
            if (key === 'timestamp') {
                value = formatDate(new Date(value));
            }
            let cell = newRow.insertCell();
            let textNode = document.createTextNode(value);
            cell.appendChild(textNode);
        });

        newRow.addEventListener('click', () => {
            const rowDate = formatDate(new Date(rowData.timestamp));
            toggleRowSelectionAndMarker(index, rowData.latitude, rowData.longitude, rowData.type, rowData.value, rowData.timestamp);
        });
    });
}

async function fetchWeatherData(latitude, longitude, timestamp) {
    const apiKey = '944b9e2ac7ada18eddd9b10849830d6d';
    const url = `https://api.openweathermap.org/data/3.0/onecall/timemachine?lat=${latitude}&lon=${longitude}&dt=${timestamp}&appid=${apiKey}`;
    try {
        const response = await fetch(url);
        const data = await response.json();
        if (response.ok) {
            return data.data[0].pressure;
        } else {
            throw new Error(`Weather API responded with status: ${response.status}`);
        }
    } catch (error) {
        console.error('Error fetching weather data:', error);
        return null;
    }
}

function calculateAltitude(pressureFlightLevel, pressureSeaLevel = 1013.25) {
    if (isNaN(pressureFlightLevel) || pressureFlightLevel <= 0 ||
        isNaN(pressureSeaLevel) || pressureSeaLevel <= 0) {
        console.error('Invalid pressure values');
        return NaN;
    }

    const altitudeFeet = ((Math.pow(10, (Math.log10(pressureFlightLevel / pressureSeaLevel) / 5.2558797)) - 1) / (-6.8755856 * Math.pow(10, -6)));
    console.log(altitudeFeet);
    return altitudeFeet;
}

async function toggleRowSelectionAndMarker(rowId, latitude, longitude, type, value, time, timestamp) {
    const row = document.querySelector(`tr[data-row-id="${rowId}"]`);
    const isSelected = row.classList.contains('selected');
    const existingIndex = selectedRowIds.indexOf(rowId);
    const unixdate = Math.floor(new Date(time).getTime() / 1000);
    const pressureSeaLevel = await fetchWeatherData(latitude, longitude, unixdate);
    const altitude = calculateAltitude(value, pressureSeaLevel);

    if (isSelected) {
        row.classList.remove('selected');
        if (existingIndex > -1) {
            markers[existingIndex].setMap(null);
            markers.splice(existingIndex, 1);
            selectedRowIds.splice(existingIndex, 1);
        }
    }
    else {
        if (markers.length >= 5) {
            const oldestRowId = selectedRowIds[0];
            document.querySelector(`tr[data-row-id="${oldestRowId}"]`).classList.remove('selected');
            markers[0].setMap(null);
            markers.shift();
            selectedRowIds.shift();
        }
        row.classList.add('selected');
        var info='';
        if (type == 'PRESSURE' || type=='pressure'){
            info = `<div>
    <h1>Reading Info</h1>
    <p><b>Latitude:</b> ${latitude}</p>
    <p><b>Longitude:</b> ${longitude}</p>
    <p><b>${type}:</b> ${value}</p>
    <p><b>Estimated Altitude:</b> ${altitude} feet</p>
    <p><b>time:</b> ${time}</p>
    </div>`;
        }
        else{
         info = `<div>
    <h1>Reading Info</h1>
    <p><b>Latitude:</b> ${latitude}</p>
    <p><b>Longitude:</b> ${longitude}</p>
    <p><b>${type}:</b> ${value}</p>
    <p><b>time:</b> ${time}</p>
    </div>`;
        }


        const infoBubble = new google.maps.InfoWindow({
            content: info
        });

        const newMarker = new google.maps.Marker({
            position: { lat: latitude, lng: longitude },
            map: window.map,
        });

        newMarker.addListener('click', () => {
            infoBubble.open({
                anchor: newMarker,
                map: window.map,
                shouldFocus: false
            });
        });

        window.map.setCenter({ lat: latitude, lng: longitude });
        markers.push(newMarker);
        selectedRowIds.push(rowId);
    }
}

function clearMarkers() {
    markers.forEach(marker => marker.setMap(null));
    markers = [];
    selectedRowIds = [];

    document.querySelectorAll('tr.selected').forEach(row => {
        row.classList.remove('selected');
    });
}



async function populateDeviceDropdown() {
    try {
        const deviceData = await fetchDeviceData();
        const deviceDropdown = document.getElementById('deviceDropdown');
        deviceDropdown.addEventListener('change', () => {
            clearMarkers(); // Call clearMarkers when a new device is selected
            displayData(); // Fetch and display the data for the new device
        });
        deviceData.forEach(device => {
            const option = document.createElement('option');
            option.value = device.device_id;
            option.textContent = device.notes;
            deviceDropdown.appendChild(option);
        });
    } catch (error) {
        console.error('Error populating device dropdown:', error);
    }
    
}



displayData();
populateDeviceDropdown();
