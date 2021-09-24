
from solid_access import SolidAccess
import matplotlib.pyplot as plt
import time
import pytz
import json
import datetime
import rdflib
import rdflib_jsonld
import yaml


#Plot data then upload files to solid pod
#
def graphData(sc,fileList):
  plotData(sc,fileList)
  sc.uploadFile('moisture.png','image/png')
  sc.uploadFile('temperature.png','image/png')
  sc.uploadFile('rssi.png','image/png')
  sc.uploadFile('garden.html','text/html')


#Parse RDF and create plots
#
def plotData(sc,source_files):
  tsList = []
  dateList = []
  moistList = []
  moistMinList = []
  moistMaxList = []
  motorStatusList = []
  rssiList = []
  tempList = []
  gmtTZ = pytz.timezone("GMT")
  localTZ = pytz.timezone("US/Central")

  graph = rdflib.Graph()
  for filename in source_files:
    print("DATA FROM:",filename)
    resp = sc.readFile(filename,'text/turtle')
    graph.parse(data=resp,format='ttl')

    querystr = """SELECT ?s ?p ?o WHERE { ?s ?p ?o } ORDER BY ASC(?s) """

    indx = 0
    for s,p,o in graph.query(querystr):
      if(str(p) == 'extern:rssi'):
        s = s.replace("-"," ")
        gmtDate = datetime.datetime.strptime(s,': %a, %d %b %Y %H:%M:%S %Z')
        localDate = gmtTZ.localize(gmtDate).astimezone(localTZ)
        timestamp = gmtDate.timestamp()
        localTime = str(localDate)[5:16]
        dateList.append(localTime)
        tsList.append(timestamp)
        rssiList.append(int(o))
        indx += 1
      elif(str(p) == 'extern:moisture'):
        moist = int(o);
        moistList.append(round(moist))
      elif(str(p) == 'extern:moisture_min'):
        moist_min = int(o);
        moistMinList.append(round(moist_min))
      elif(str(p) == 'extern:moisture_max'):
        moist_max = int(o);
        moistMaxList.append(round(moist_max))
      elif(str(p) == 'extern:motor_status'):
        motor_status = int(o);
        motorStatusList.append(round(motor_status))
      elif(str(p) == 'extern:temperature'):
        #tempf = (int(o)*(9/5) +32); #degF
        temp = (int(o)); #degC
        tempList.append(round(temp))

  if(len(tsList)>0):
    print("MOST RECENT :",filename,tsList[-1],dateList[-1],moistList[-1],moistMinList[-1],moistMaxList[-1])
  else:
    print("NO DATA YET")

  #clean out most date labels leaving every 4th one
  for indx,d in enumerate(dateList):
    if (indx % 4 != 0):
      dateList[indx] = ""

  MSoff,MSon = calculateStatusBands(motorStatusList,tsList)

  #Moisture
  plt.plot(tsList,moistList,label='Moisture')
  plt.plot(tsList,moistMinList,label='Moisture Min')
  plt.plot(tsList,moistMaxList,label='Moisture Max')
  for indx,mson in enumerate(MSon):
    plt.axvspan(mson,MSoff[indx],color="aqua",alpha=0.3)
  plt.xticks(tsList,dateList,rotation=45,fontsize=5)
  plt.legend()
  plt.savefig('moisture.png',format='PNG')

  #Temperature
  plt.clf();
  plt.plot(tsList,tempList,label='Temperature \u2103')
  plt.xticks(tsList,dateList,rotation='vertical')
  plt.xticks(tsList,dateList,rotation=45,fontsize=5)
  plt.legend()
  plt.savefig('temperature.png',format='PNG')

  #WiFi Signal Strength
  plt.clf();
  plt.plot(tsList,rssiList,label='WiFi dB')
  plt.axhspan(-50,-70,color="lightgreen",alpha=0.8) #4 bars
  plt.axhspan(-70.1,-85,color="lightgreen",alpha=0.6) #3 bars
  plt.axhspan(-85.1,-100,color="lightgreen",alpha=0.4) #2 bars
  plt.axhspan(-100.1,-110,color="lightgreen",alpha=0.2) #1 bar
  plt.xticks(tsList,dateList,rotation=45,fontsize=5)
  plt.legend()
  plt.savefig('rssi.png',format='PNG')


#Utility to calculate motor status vertical bands
#
def calculateStatusBands(motorStatusList,tsList):
  #MOTOR_STATUS Preprocessing
  MSoff = []
  MSon = []
  prevMS = 0
  lastIndex = 0
  for indx, ms in enumerate(motorStatusList):
    if (ms > prevMS):
      MSon.append(tsList[indx])
      lastIndex = indx
    elif (ms < prevMS):
      MSoff.append(tsList[indx])
    prevMS = ms
  if(len(MSoff) < (indx+1)): #IF STATUS IS ON AT END OF DATA
      MSoff.append(tsList[-1]) #SET END OF BAND TO END OF TIMESTAMP
  return MSoff,MSon


#Set min and max levels in commands CSV file
#
def createCsvCmd(sc,filename,the_min,the_max):
  session_cookie = sc.signIn()
  headerDate,statusCode = sc.getFileHeader(filename)
  ct = headerDate.replace(" ","-").replace(",","")
  csv_data = 'SETTING,'+ct+','+str(the_min)+','+str(the_max)
  sc.putFile(filename,'text/csv',csv_data)


#Verify min and max values in commands CSV file
#
def getCsvCmd(sc,filename):
  session_cookie = sc.signIn()
  headerDate,statusCode = sc.getFileHeader(filename)
  print("HEADER DATE "+headerDate+" "+str(statusCode))
  new_min = -1
  new_max = -1
  if(statusCode==200):
    resp = sc.readFile(filename,'text/csv')
    cmd,dateStr,new_min,new_max = resp.split(",")
    print(cmd,dateStr,new_min,new_max)
  else:
    print("FAILURE TO READ:",filename)


#Edit to set MIN and MAX moisture levels.
#Edit to denote which dates data set to process.
#
#Sign in to Solid Pod
#Read credentials from YML file.
#Set values and create plots from reported data
#Sign out from Solid Pod
#
#This program is counting on a data file on the Solid Pod (generated by Arduino) to be available for it to read. 
def main():
  with open('credentials.yaml') as cred:
    try:
      credentials = yaml.safe_load(cred)
      uri = credentials["base_url"]
      un = credentials["username"]
      pw = credentials["password"]
      sc = SolidAccess(uri,un,pw)
      session_cookie = sc.signIn()

      cmdfilename = 'commands.csv'

      #turn watering off but still display distinct min/max lines
      #createCsvCmd(uri,un,pw,cmdfilename,0,50)
      #start watering if moisture below 265, stop when 340
      #createCsvCmd(uri,un,pw,cmdfilename,265,340)
      createCsvCmd(sc,cmdfilename,340,385)
      #verify command
      getCsvCmd(sc,cmdfilename)
      #MOTOR_ON SAMPLE DATA
      dayFileList = ['garden_20210902.ttl']
      #dayFileList = ['garden_20210930.ttl']
      graphData(sc,dayFileList)
      sc.signOut()
    except yaml.YAMLError as exc:
      print(exc)


if __name__ == "__main__":
  main()


