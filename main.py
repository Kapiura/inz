import requests as req
import json
import datetime

class Requestor:
    def __init__(self):
        self.url = 'https://www.wroclaw.pl/open-data/api/action/datastore_search?resource_id=a9b3841d-e977-474e-9e86-8789e470a85a'
        self.data = req.get(self.url)
        self.jsonData = self.data.json()
        self.records = self.jsonData['result']['records']

    def getRecords(self):
        return self.records

class Vehicle:
    updateDate: datetime.datetime
    def __init__(self, record : list):
        self.id = record['_id']
        self.brigade = record['Brygada']
        self.areaNr = record['Nr_Rej']
        self.updateDate = record['Data_Aktualizacji']
        self.sideNr = record['Nr_Boczny']
        self.lastPosWidth = record['Ostatnia_Pozycja_Szerokosc']
        self.lastPosLength = record['Ostatnia_Pozycja_Dlugosc']

    def print(self):
        print('!==POJAZD==!')
        print('\tid: ', self.id)
        print('\tBrygada: ', self.brigade)
        print('\tNumer Rejonu', self.areaNr)
        print('\tData aktualizacji', self.updateDate)
        print('\tNumer Boczny', self.sideNr)



def main():
    r = Requestor()
    records = r.getRecords()
    vehicles = []
    for record in records:
        vehicles.append(Vehicle(record))
    for v in vehicles:
        v.print()
    # r = req.get(url)
    # r_json = r.json()
    # result = r_json['result']['records']
    #
    # for x in result:
    #     print(x)



if __name__ == "__main__":
    main()
