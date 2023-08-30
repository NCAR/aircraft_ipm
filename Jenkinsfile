pipeline {
  agent none
  triggers {
    pollSCM('H/5 * * * *')
  }
  stages {
    stage('Shell script 0') {
      agent {
        node {
          label 'UbuntuBionic32'
        }
      }
      steps {
        sh 'scons'
      }
    }
    stage('Shell script 1') {
      agent {
        node {
          label 'CentOS8'
        }
      }
      steps {
        sh 'scons'
      }
    }
  }
  post {
    failure {
      emailtext 
      to: 'janine@ucar.edu', 
      subject: 'iPM Jenkinsfile build failed',
       body: 'See attached build console output', 
      attachLog: true
    }
  }
  options {
    buildDiscarder(logRotator(numToKeepStr: '6'))
  }
}
