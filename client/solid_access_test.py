
from solid_access import SolidAccess
import yaml

#print(dir(sc)) #SHOW METHODS
#print(sc.currentTime())

def newTest():
  with open('credentials.yaml') as cred:
    try:
      credentials = yaml.safe_load(cred)
      scx = SolidAccess(credentials["base_url"],credentials["username"],credentials["password"])
      session_cookie = scx.signIn()
      print("SESSION COOKIE:",session_cookie)

      filename = 'test.txt'
      content_type = 'text/csv'
      resp = scx.putFile(filename,content_type,'some test data')
      print("CREATE RESP:",resp)

      header_date,status = scx.getFileHeader(filename)
      print("HEADER DATE:",header_date,status)

      readData = scx.readFile(filename,content_type)
      print("READ DATA:",readData)

      delData = scx.deleteFile(filename,content_type)
      print("DEL DATA:",delData)

      filename = 'moisture.png'
      resp = scx.uploadFile(filename,'image/png')
      print("UPLOAD:",resp)

      header_date,status = scx.getFileHeader(filename)
      print("HEADER DATE:",header_date,status)

      delData = scx.deleteFile(filename,content_type)
      print("DEL DATA:",delData)

      so = scx.signOut()
      print("SIGN OUT:",so)
    except yaml.YAMLError as exc:
      print(exc)


newTest()


