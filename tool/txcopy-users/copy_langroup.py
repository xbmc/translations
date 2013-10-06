#!/usr/bin/env python

# prequisite: requests python module 

import json
import requests

# You know what you have to do
USER = 'TeamXBMC'
PASSWORD = 'Frodo2013'

# You know what you have to do
SOURCE_PROJECT = 'XBMC-Main-Frodo'
TARGET_PROJECT = 'xbmc-main'

def project_teams_url(project_slug):
    return 'https://www.transifex.com/api/2/project/%s/languages/' % project_slug

def main():
    auth = requests.auth.HTTPBasicAuth(USER, PASSWORD)

    response = requests.get(project_teams_url(SOURCE_PROJECT), auth=auth)
    teams = response.json()

    headers ={
        'Content-type': 'application/json',
        'Accept': 'text/plain'
    }
    create_team_url = project_teams_url(TARGET_PROJECT)

    for team in teams:
        data = json.dumps(team)
        requests.post(create_team_url, data=data, auth=auth, headers=headers)

if __name__ == '__main__':
    main()
