import datetime
import logging
import requests

class SolidAccess:
  def __init__(self,base_url,username,password):
    self.base_url = base_url
    self.username = username
    self.password = password
    self.signin_url = '/login/password'
    self.signout_url = '/logout'

  def signIn(self):
    signin_data = {'username': self.username, 'password': self.password}
    signin_resp = requests.post(self.base_url+self.signin_url, data = signin_data)
    #print(requests.request)
    self.session_cookie = signin_resp.cookies
    return self.session_cookie

  def putFile(self,filename,content_type,file_data):
    put_header = {'content-type':content_type} #'text/csv'}
    r = requests.put(self.base_url+'/'+filename,cookies=self.session_cookie,headers=put_header,data=file_data)
    return r.text

  def getFileHeader(self,filename):
    r = requests.head(self.base_url+'/'+filename,cookies=self.session_cookie)
    return r.headers['Date'],r.status_code

  def readFile(self,filename,content_type):
    read_header = {'Accept':content_type}
    r = requests.get(self.base_url+'/'+filename,headers=read_header,cookies=self.session_cookie)
    resp = r.text
    return resp

  def uploadFile(self,filename,content_type):
    put_header = {'content-type':content_type} #'image/png','text/html','text/csv'}
    with open(filename,'rb') as f:
      r = requests.put(self.base_url+'/'+filename,cookies=self.session_cookie,data=f,headers=put_header)
      return r.text
      #return r.status_code

  def deleteFile(self,filename,content_type):
    put_header = {'content-type':content_type} #'text/turtle'}
    r = requests.delete(self.base_url+'/'+filename,cookies=self.session_cookie,headers=put_header)
    return r.text

  def signOut(self):
    r = requests.get(self.base_url+self.signout_url,cookies=self.session_cookie)
    return r.status_code
    #return r.text

######
#  def patchSparqlFile(base_url,session_cookie,filename,content_type,patch_text):
#    patch_header = {'content-type':content_type} #'application/sparql-update'}
#    r = requests.patch(base_url+'/'+filename,cookies=session_cookie,headers=patch_header,data=patch_text)
#    return r.headers['Date']

