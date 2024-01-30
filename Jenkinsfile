pipeline {
    parameters {
        // SCAN Variables
        string(name: 'SCANOSS_API_TOKEN_ID', defaultValue:"scanoss-token", description: 'The reference ID for the SCANOSS API TOKEN credential')
        
        string(name: 'SCANOSS_SBOM_IDENTIFY', defaultValue:"sbom.json", description: 'SCANOSS SBOM Identify filename')
        
        string(name: 'SCANOSS_SBOM_IGNORE', defaultValue:"sbom-ignore.json", description: 'SCANOSS SBOM Ignore filename')
        
        // JIRA Variables
        string(name: 'JIRA_URL', defaultValue:"https://scanoss.atlassian.net/" , description: 'Jira URL')
        
        string(name: 'JIRA_PROJECT_KEY', defaultValue:"TESTPROJ" , description: 'Jira Project Key')
                
        booleanParam(name: 'CREATE_JIRA_ISSUE', defaultValue: true, description: 'Enable Jira reporting')
        
        booleanParam(name: 'ABORT_ON_POLICY_FAILURE', defaultValue: false, description: 'Abort Pipeline on pipeline Failure')
        
    }
    agent any
      stages {
        stage('Git Checkout') {
            steps {
                script {
                    dir('repository') {
                        git branch: 'main',
                            credentialsId: 'gh-token',
                            url: 'https://github.com/agustingroh/jenkins-test-personal'
                    }
                        
                }
            }
        }
        stage('Policy setup') {
            steps {
                //TODO: Remove credential when policies are public
                withCredentials([string(credentialsId: 'policy-token' , variable: 'SCANOSS_POLICY_TOKEN')]) {
                    script {
                        def command = 'curl -H "Authorization: Bearer $SCANOSS_POLICY_TOKEN" -L -o policy_check_script.js https://raw.githubusercontent.com/scanoss/jenkins-pipeline-example/main/copyleft-policy.js'
                        echo command
                        def response = sh(script: command, returnStdout: true).trim() 
                    }
                }
            }
        }
        stage('Scan') {
            agent {
                docker {
                    image 'ghcr.io/scanoss/scanoss-py:v1.9.0'
                    args '--entrypoint='
                    // Run the container on the node specified at the
                    // top-level of the Pipeline, in the same workspace,
                    // rather than on a new node entirely:
                    reuseNode true
                }
            }
            steps {
                
                withCredentials([string(credentialsId: params.SCANOSS_API_TOKEN_ID , variable: 'SCANOSS_API_TOKEN')]) {
                    dir('repository') {
                        script {

                            sh '''

                            SBOM_IDENTIFY=""
                            if [ -f $SCANOSS_SBOM_IDENTIFY ]; then SBOM_IDENTIFY="--identify $SCANOSS_SBOM_IDENTIFY" ; fi

                            SBOM_IGNORE=""
                            if [ -f $SCANOSS_SBOM_IGNORE ]; then SBOM_IGNORE="--ignore $SCANOSS_SBOM_IGNORE" ; fi


                            CUSTOM_URL=""
                            if [ ! -z $SCANOSS_API_URL ]; then CUSTOM_URL="--apiurl $SCANOSS_API_URL"; else CUSTOM_URL="--apiurl https://osskb.org/api/scan/direct" ; fi

                            CUSTOM_TOKEN=""
                            if [ ! -z $SCANOSS_API_TOKEN ]; then CUSTOM_TOKEN="--key $SCANOSS_API_TOKEN" ; fi


                            scanoss-py scan $CUSTOM_URL $CUSTOM_TOKEN $SBOM_IDENTIFY $SBOM_IGNORE --output ../scanoss-results.json .
                            '''
                        }
                    }
                }
            }
        }
        stage('Upload Artifacts') {
            steps {
                archiveArtifacts artifacts: 'scanoss-results.json', onlyIfSuccessful: true

            }
        }
        stage('Process Scan Results') {
            agent {
                docker {
                     image 'node:20.11.0-alpine3.19' 
                     reuseNode true
                }
            }
            steps { 
                script {
                    try{                            
                        env.check_result = sh(
                                returnStatus: true,
                                script: 'node policy_check_script.js'
                            )
                        if (params.ABORT_ON_POLICY_FAILURE && env.check_result != '0') {
                            currentBuild.result = "FAILURE"
                        }
                        echo "$check_result"
                    }catch(e){
                        echo e.getMessage()
                        if (params.ABORT_ON_POLICY_FAILURE) {
                            currentBuild.result = "FAILURE"
                        }                
                    }
                }
            }
        }
        stage('Publish CSV Reports') {
            steps {
                publishReport name: "Scan Results", displayType: "dual", provider: csv(id: "report-summary", pattern: "data.csv")
            }
        }
        
        stage('Jira Issue'){
            when {
                allOf {
                    expression { params.CREATE_JIRA_ISSUE == true }
                    expression { env.check_result != '0'}
                }
            }
            steps {
              withCredentials([usernamePassword(credentialsId: 'jira-token',usernameVariable: 'USERNAME', passwordVariable: 'PASSWORD')]) {
                    script {
                       
                        def copyLeft = sh(script: "tail -n +2 data.csv | cut -d',' -f1", returnStdout: true)
                        
                        copyLeft = copyLeft +  "\n${BUILD_URL}"
                            
                        def JSON_PAYLOAD =  [
                             fields : [
                                project : [
                                    key: params.JIRA_PROJECT_KEY
                                ],
                                summary : 'Components with Copyleft licenses found',
                                description: copyLeft,
                                issuetype: [
                                    name: 'Bug'
                                ]
                            ]
                        ]
    
                        def jsonString = groovy.json.JsonOutput.toJson(JSON_PAYLOAD)
                        
                        createJiraIssue(PASSWORD, USERNAME, params.JIRA_URL, jsonString)
                    }
              }
              
           }
        }
    }    
        
}    

def createJiraIssue(jiraToken, jiraUsername, jiraAPIEndpoint, payload) {
    env.TOKEN = jiraToken
    env.USER = jiraUsername
    env.JIRA_ENDPOINT_URL = jiraAPIEndpoint + '/rest/api/2/issue/'
    env.PAYLOAD = payload

    try {
        def command = """
            curl -u '${USER}:${TOKEN}' -X POST --data '${PAYLOAD}' -H 'Content-Type: application/json' '${JIRA_ENDPOINT_URL}' 
        """

        def response = sh(script: command, returnStdout: true).trim()
        echo "Response: ${response}"
 
    } catch (Exception e) {
        echo e
    }
}