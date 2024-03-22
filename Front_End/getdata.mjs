



  export async function fetchData() {
    try {
      const response = await fetch('https://superdomain.asuscomm.com/nodered/getdata');
      const data = await response.json();
      return data;
    } catch (error) {
      console.error('Error fetching data:', error);
      return []; 
    }
  }
  
  

