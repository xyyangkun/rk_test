import QtQuick 2.12
import QtQuick.Window 2.12

Window {
    visible: true
    width: 1920
    height: 1080
    title: qsTr("Hello World")
    /*color: "transparent"*/

    /*
    Image{
        id: image
        width:1560
        height:1190
        source: "./RC.png"
    }

    Timer {
            id:timer
            interval: 50; running: true; repeat: true
            onTriggered: {
                image.visible = !image.visible;
                console.log("111");
            }
        }

        Component.onCompleted: {
                    timer.start();
                }
                */

    Rectangle {
          width: 1920; height: 1080
          ListModel {
               id: fruitModel
               ListElement {
                   name: "Apple"
                   cost: 3.141592653589793238462643383279502884197169399375105820974944
               }
               ListElement {
                   name: "Orange"
                   cost: 3.141592653589793238462643383279502884197169399375105820974944
               }
               ListElement {
                   name: "Banana"
                   cost: 3.141592653589793238462643383279502884197169399375105820974944
               }
               ListElement {
                   name: "Apple"
                   cost: 3.141592653589793238462643383279502884197169399375105820974944
               }
               ListElement {
                   name: "Orange"
                   cost: 3.141592653589793238462643383279502884197169399375105820974944
               }
               ListElement {
                   name: "Banana"
                   cost: 3.141592653589793238462643383279502884197169399375105820974944
               }
               ListElement {
                   name: "Apple"
                   cost: 3.141592653589793238462643383279502884197169399375105820974944
               }
               ListElement {
                   name: "Orange"
                   cost: 3.141592653589793238462643383279502884197169399375105820974944
               }
               ListElement {
                   name: "Banana"
                   cost: 3.141592653589793238462643383279502884197169399375105820974944
               }
               ListElement {
                   name: "Apple"
                   cost: 3.141592653589793238462643383279502884197169399375105820974944
               }
               ListElement {
                   name: "Orange"
                   cost: 3.141592653589793238462643383279502884197169399375105820974944
               }
               ListElement {
                   name: "Banana"
                   cost: 3.141592653589793238462643383279502884197169399375105820974944
               }
               ListElement {
                   name: "Apple"
                   cost: 3.141592653589793238462643383279502884197169399375105820974944
               }
               ListElement {
                   name: "Orange"
                   cost: 3.141592653589793238462643383279502884197169399375105820974944
               }
               ListElement {
                   name: "Banana"
                   cost: 3.141592653589793238462643383279502884197169399375105820974944
               }
           }

          Component {
              id: fruitDelegate
              Row {
                  spacing: 80
                  Text { text: name ;font.pixelSize: 74}
                  Text { text: '$' + cost;font.pixelSize: 74 }
              }
          }

          ListView {
//              Component {
//                  id: contactsDelegate
//                  Rectangle {
//                      id: wrapper
//                      width: 300
//                      height: contactInfo.height
//                      color: ListView.isCurrentItem ? "black" : "red"
//                      Text {
//                          id: contactInfo
//                          text: name + ": " + number
//                          color: wrapper.ListView.isCurrentItem ? "red" : "black"
//                      }
//                  }
//              }
//              anchors.fill: parent
//              model: fruitModel
//              delegate: fruitDelegate

              width: 1920; height: 1080

                    contentWidth: 3200
                    flickableDirection: Flickable.AutoFlickDirection


                    model: fruitModel
                    delegate: fruitDelegate

          }
      }

}
