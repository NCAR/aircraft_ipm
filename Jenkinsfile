pipeline {
  agent {
     node { 
        label 'UbuntuBionic32'
        } 
  }
  triggers {
    pollSCM('H/5 * * * *')
  }
  stages {
    stage('Checkout Scm') {
      steps {
        git(credentialsId: '78b46507-ad3f-4a59-93df-11a69e10cd53', url: 'eolJenkins:NCAR/aircraft_ipm.git')
      }
    }
    stage('Shell script 0') {
      steps {
        sh 'scons'
      }
    }
    stage('Shell script 1') {
      steps {
        sh 'scons tests'
        sh 'tests/g_test'
      }
    }
  }
  post {
    failure {
      mail(subject: 'iPM Jenkinsfile build failed', body: 'See build console output within jenkins for details', to: 'janine@ucar.edu')
    }

  }
  options {
    buildDiscarder(logRotator(numToKeepStr: '6'))
  }
}
