import { fetchData } from './getdata.mjs';

async function displayData() {
  const data = await fetchData();
  insertDataToGrid(data);
}
function insertDataToGrid(data) {
  const tableBody = document.getElementById('dataGrid').getElementsByTagName('tbody')[0];
  tableBody.innerHTML = '';

  data.slice(0, 100).forEach(rowData => {
    let newRow = tableBody.insertRow();
    
    Object.values(rowData).forEach(text => {
      let newCell = newRow.insertCell();
      let newText = document.createTextNode(text);
      newCell.appendChild(newText);
    });
    
    newRow.addEventListener('click', () => {
      document.querySelectorAll('.selected').forEach(selectedRow => {
        selectedRow.classList.remove('selected');
      });

      newRow.classList.add('selected');
      window.addMarker(rowData.latitude, rowData.longitude);
    });
  });
}

displayData();

