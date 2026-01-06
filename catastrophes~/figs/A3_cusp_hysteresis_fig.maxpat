{
  "patcher": {
    "fileversion": 1,
    "appversion": {
      "major": 8,
      "minor": 6,
      "revision": 5,
      "architecture": "x64",
      "modernui": 1
    },
    "classnamespace": "box",
    "rect": [
      657.0,
      89.0,
      563.0,
      288.0
    ],
    "bglocked": 0,
    "openinpresentation": 1,
    "default_fontsize": 12.0,
    "default_fontface": 0,
    "default_fontname": "Arial",
    "gridonopen": 1,
    "gridsize": [
      15.0,
      15.0
    ],
    "gridsnaponopen": 1,
    "objectsnaponopen": 1,
    "statusbarvisible": 2,
    "toolbarvisible": 1,
    "lefttoolbarpinned": 0,
    "toptoolbarpinned": 0,
    "righttoolbarpinned": 0,
    "bottomtoolbarpinned": 0,
    "toolbars_unpinned_last_save": 0,
    "tallnewobj": 0,
    "boxanimatetime": 200,
    "enablehscroll": 1,
    "enablevscroll": 1,
    "devicewidth": 0.0,
    "description": "",
    "digest": "",
    "tags": "",
    "style": "",
    "subpatcher_template": "",
    "assistshowspatchername": 0,
    "boxes": [
      {
        "box": {
          "id": "obj-22",
          "maxclass": "button",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            "bang"
          ],
          "parameter_enable": 0,
          "patching_rect": [
            27.0,
            7.0,
            24.0,
            24.0
          ],
          "presentation": 1,
          "presentation_rect": [
            10.0,
            10.0,
            24.0,
            24.0
          ]
        }
      },
      {
        "box": {
          "id": "obj-32",
          "maxclass": "comment",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            503.0,
            9.0,
            235.0,
            20.0
          ],
          "text": "catastrophes~ — cusp~ test patch (v1.0.0)"
        }
      },
      {
        "box": {
          "id": "obj-15",
          "linecount": 2,
          "maxclass": "comment",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            442.0,
            185.0,
            50.0,
            33.0
          ],
          "text": "nroots (1/3)"
        }
      },
      {
        "box": {
          "format": 6,
          "id": "obj-29",
          "maxclass": "flonum",
          "numinlets": 1,
          "numoutlets": 2,
          "outlettype": [
            "",
            "bang"
          ],
          "parameter_enable": 0,
          "patching_rect": [
            442.0,
            161.0,
            50.0,
            22.0
          ]
        }
      },
      {
        "box": {
          "id": "obj-30",
          "maxclass": "newobj",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            "float"
          ],
          "patching_rect": [
            442.0,
            137.0,
            81.0,
            22.0
          ],
          "text": "snapshot~ 10"
        }
      },
      {
        "box": {
          "id": "obj-16",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            79.0,
            81.0,
            50.0,
            22.0
          ],
          "text": "set $1"
        }
      },
      {
        "box": {
          "format": 6,
          "id": "obj-13",
          "maxclass": "flonum",
          "numinlets": 1,
          "numoutlets": 2,
          "outlettype": [
            "",
            "bang"
          ],
          "parameter_enable": 0,
          "patching_rect": [
            27.0,
            57.0,
            50.0,
            22.0
          ]
        }
      },
      {
        "box": {
          "id": "obj-14",
          "maxclass": "newobj",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            "float"
          ],
          "patching_rect": [
            79.0,
            57.0,
            81.0,
            22.0
          ],
          "text": "snapshot~ 10"
        }
      },
      {
        "box": {
          "id": "obj-12",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            577.0,
            56.0,
            70.0,
            22.0
          ],
          "text": "loadmess 1"
        }
      },
      {
        "box": {
          "id": "obj-10",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            328.0,
            57.0,
            84.0,
            22.0
          ],
          "text": "loadmess -0.5"
        }
      },
      {
        "box": {
          "id": "obj-11",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            27.0,
            33.0,
            137.0,
            22.0
          ],
          "text": "-0.5, 0.5 4000 0.5 1000 -0.5 4000"
        }
      },
      {
        "box": {
          "id": "obj-8",
          "maxclass": "newobj",
          "numinlets": 2,
          "numoutlets": 2,
          "outlettype": [
            "signal",
            "bang"
          ],
          "patching_rect": [
            27.0,
            81.0,
            34.0,
            22.0
          ],
          "text": "line~"
        }
      },
      {
        "box": {
          "id": "obj-7",
          "maxclass": "ezdac~",
          "numinlets": 2,
          "numoutlets": 0,
          "patching_rect": [
            27.0,
            222.0,
            45.0,
            45.0
          ]
        }
      },
      {
        "box": {
          "id": "obj-1",
          "maxclass": "comment",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            525.0,
            185.0,
            38.0,
            20.0
          ],
          "text": "x_out"
        }
      },
      {
        "box": {
          "format": 6,
          "id": "obj-4",
          "maxclass": "flonum",
          "numinlets": 1,
          "numoutlets": 2,
          "outlettype": [
            "",
            "bang"
          ],
          "parameter_enable": 0,
          "patching_rect": [
            525.0,
            161.0,
            50.0,
            22.0
          ]
        }
      },
      {
        "box": {
          "id": "obj-5",
          "maxclass": "newobj",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            "float"
          ],
          "patching_rect": [
            525.0,
            137.0,
            81.0,
            22.0
          ],
          "text": "snapshot~ 10"
        }
      },
      {
        "box": {
          "fontface": 2,
          "id": "obj-71",
          "maxclass": "comment",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            490.0,
            58.0,
            37.0,
            20.0
          ],
          "text": "width"
        }
      },
      {
        "box": {
          "id": "obj-69",
          "maxclass": "comment",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            6.0,
            58.0,
            19.0,
            20.0
          ],
          "text": "α"
        }
      },
      {
        "box": {
          "id": "obj-67",
          "maxclass": "comment",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            258.0,
            58.0,
            19.0,
            20.0
          ],
          "text": "β"
        }
      },
      {
        "box": {
          "id": "obj-9",
          "linecount": 2,
          "maxclass": "comment",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            110.0,
            185.0,
            54.0,
            33.0
          ],
          "text": "Exit r1\n(middle)"
        }
      },
      {
        "box": {
          "format": 6,
          "id": "obj-3",
          "maxclass": "flonum",
          "numinlets": 1,
          "numoutlets": 2,
          "outlettype": [
            "",
            "bang"
          ],
          "parameter_enable": 0,
          "patching_rect": [
            110.0,
            161.0,
            50.0,
            22.0
          ]
        }
      },
      {
        "box": {
          "id": "obj-6",
          "maxclass": "newobj",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            "float"
          ],
          "patching_rect": [
            110.0,
            137.0,
            81.0,
            22.0
          ],
          "text": "snapshot~ 10"
        }
      },
      {
        "box": {
          "id": "obj-54",
          "maxclass": "comment",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            359.0,
            185.0,
            75.0,
            20.0
          ],
          "text": "Discriminant"
        }
      },
      {
        "box": {
          "format": 6,
          "id": "obj-55",
          "maxclass": "flonum",
          "numinlets": 1,
          "numoutlets": 2,
          "outlettype": [
            "",
            "bang"
          ],
          "parameter_enable": 0,
          "patching_rect": [
            359.0,
            161.0,
            50.0,
            22.0
          ]
        }
      },
      {
        "box": {
          "id": "obj-56",
          "maxclass": "newobj",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            "float"
          ],
          "patching_rect": [
            359.0,
            137.0,
            81.0,
            22.0
          ],
          "text": "snapshot~ 10"
        }
      },
      {
        "box": {
          "id": "obj-17",
          "linecount": 2,
          "maxclass": "comment",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            193.0,
            185.0,
            55.0,
            33.0
          ],
          "text": "Exit r2 \n(highest)"
        }
      },
      {
        "box": {
          "format": 6,
          "id": "obj-19",
          "maxclass": "flonum",
          "numinlets": 1,
          "numoutlets": 2,
          "outlettype": [
            "",
            "bang"
          ],
          "parameter_enable": 0,
          "patching_rect": [
            193.0,
            161.0,
            50.0,
            22.0
          ]
        }
      },
      {
        "box": {
          "id": "obj-20",
          "maxclass": "newobj",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            "float"
          ],
          "patching_rect": [
            193.0,
            137.0,
            81.0,
            22.0
          ],
          "text": "snapshot~ 10"
        }
      },
      {
        "box": {
          "id": "obj-23",
          "linecount": 4,
          "maxclass": "comment",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            259.0,
            185.0,
            85.0,
            60.0
          ],
          "text": "State\n(branch)\n0 → r0,\n1 → r2"
        }
      },
      {
        "box": {
          "format": 6,
          "id": "obj-24",
          "maxclass": "flonum",
          "numinlets": 1,
          "numoutlets": 2,
          "outlettype": [
            "",
            "bang"
          ],
          "parameter_enable": 0,
          "patching_rect": [
            276.0,
            161.0,
            50.0,
            22.0
          ]
        }
      },
      {
        "box": {
          "id": "obj-25",
          "maxclass": "newobj",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            "float"
          ],
          "patching_rect": [
            276.0,
            137.0,
            81.0,
            22.0
          ],
          "text": "snapshot~ 10"
        }
      },
      {
        "box": {
          "id": "obj-26",
          "linecount": 2,
          "maxclass": "comment",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            27.0,
            185.0,
            51.0,
            33.0
          ],
          "text": "Exit r0 (lowest)"
        }
      },
      {
        "box": {
          "format": 6,
          "id": "obj-27",
          "maxclass": "flonum",
          "numinlets": 1,
          "numoutlets": 2,
          "outlettype": [
            "",
            "bang"
          ],
          "parameter_enable": 0,
          "patching_rect": [
            27.0,
            161.0,
            50.0,
            22.0
          ]
        }
      },
      {
        "box": {
          "id": "obj-28",
          "maxclass": "newobj",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            "float"
          ],
          "patching_rect": [
            27.0,
            137.0,
            81.0,
            22.0
          ],
          "text": "snapshot~ 10"
        }
      },
      {
        "box": {
          "id": "obj-35",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            "signal"
          ],
          "patching_rect": [
            525.0,
            81.0,
            31.0,
            22.0
          ],
          "text": "sig~"
        }
      },
      {
        "box": {
          "id": "obj-37",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            "signal"
          ],
          "patching_rect": [
            276.0,
            81.0,
            31.0,
            22.0
          ],
          "text": "sig~"
        }
      },
      {
        "box": {
          "format": 6,
          "id": "obj-38",
          "maxclass": "flonum",
          "numinlets": 1,
          "numoutlets": 2,
          "outlettype": [
            "",
            "bang"
          ],
          "parameter_enable": 0,
          "patching_rect": [
            525.0,
            57.0,
            50.0,
            22.0
          ]
        }
      },
      {
        "box": {
          "format": 6,
          "id": "obj-40",
          "maxclass": "flonum",
          "numinlets": 1,
          "numoutlets": 2,
          "outlettype": [
            "",
            "bang"
          ],
          "parameter_enable": 0,
          "patching_rect": [
            276.0,
            57.0,
            50.0,
            22.0
          ]
        }
      },
      {
        "box": {
          "id": "obj-2",
          "maxclass": "newobj",
          "numinlets": 3,
          "numoutlets": 7,
          "outlettype": [
            "signal",
            "signal",
            "signal",
            "signal",
            "signal",
            "signal",
            "signal"
          ],
          "patching_rect": [
            27.0,
            109.0,
            517.0,
            22.0
          ],
          "saved_object_attributes": {
            "alpha": 0.0,
            "beta": 0.0,
            "eps": 1e-12,
            "width": 1.0,
            "width_eps": 1e-12
          },
          "text": "cusp~"
        }
      },
      {
        "box": {
          "id": "obj-100",
          "maxclass": "newobj",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            "float"
          ],
          "patching_rect": [
            608.0,
            284.0,
            81.0,
            22.0
          ],
          "text": "snapshot~ 50"
        }
      },
      {
        "box": {
          "id": "obj-101",
          "maxclass": "newobj",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            "float"
          ],
          "patching_rect": [
            608.0,
            338.0,
            81.0,
            22.0
          ],
          "text": "snapshot~ 50"
        }
      },
      {
        "box": {
          "id": "obj-102",
          "maxclass": "newobj",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            "float"
          ],
          "patching_rect": [
            608.0,
            390.0,
            81.0,
            22.0
          ],
          "text": "snapshot~ 50"
        }
      },
      {
        "box": {
          "id": "obj-103",
          "maxclass": "multislider",
          "numinlets": 1,
          "numoutlets": 2,
          "outlettype": [
            "",
            ""
          ],
          "parameter_enable": 0,
          "patching_rect": [
            27.0,
            270.0,
            545.0,
            70.0
          ],
          "presentation": 1,
          "presentation_rect": [
            10.0,
            56.0,
            545.0,
            70.0
          ],
          "setminmax": [
            -2.0,
            2.0
          ],
          "setstyle": 3,
          "spacing": 1,
          "thickness": 1
        }
      },
      {
        "box": {
          "id": "obj-104",
          "maxclass": "multislider",
          "numinlets": 1,
          "numoutlets": 2,
          "outlettype": [
            "",
            ""
          ],
          "parameter_enable": 0,
          "patching_rect": [
            27.0,
            367.0,
            545.0,
            50.0
          ],
          "presentation": 1,
          "presentation_rect": [
            10.0,
            153.0,
            545.0,
            50.0
          ],
          "setminmax": [
            -3.0,
            5.0
          ],
          "setstyle": 2,
          "settype": 0,
          "spacing": 1
        }
      },
      {
        "box": {
          "id": "obj-105",
          "maxclass": "multislider",
          "numinlets": 1,
          "numoutlets": 2,
          "outlettype": [
            "",
            ""
          ],
          "parameter_enable": 0,
          "patching_rect": [
            27.0,
            438.0,
            545.0,
            50.0
          ],
          "presentation": 1,
          "presentation_rect": [
            10.0,
            231.0,
            545.0,
            50.0
          ],
          "setminmax": [
            -1.0,
            2.0
          ],
          "setstyle": 2,
          "settype": 0,
          "spacing": 1
        }
      },
      {
        "box": {
          "id": "obj-108",
          "maxclass": "comment",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            27.0,
            250.0,
            150.0,
            20.0
          ],
          "presentation": 1,
          "presentation_rect": [
            10.0,
            36.0,
            107.0,
            20.0
          ],
          "text": "x_out (scroll trace)"
        }
      },
      {
        "box": {
          "id": "obj-109",
          "maxclass": "comment",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            27.0,
            347.0,
            150.0,
            20.0
          ],
          "presentation": 1,
          "presentation_rect": [
            10.0,
            133.0,
            111.0,
            20.0
          ],
          "text": "nroots (scroll trace)"
        }
      },
      {
        "box": {
          "id": "obj-110",
          "maxclass": "comment",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            27.0,
            418.0,
            250.0,
            20.0
          ],
          "presentation": 1,
          "presentation_rect": [
            10.0,
            211.0,
            179.0,
            20.0
          ],
          "text": "state (branch index, scroll trace)"
        }
      }
    ],
    "lines": [
      {
        "patchline": {
          "destination": [
            "obj-40",
            0
          ],
          "source": [
            "obj-10",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-103",
            0
          ],
          "source": [
            "obj-100",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-104",
            0
          ],
          "source": [
            "obj-101",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-105",
            0
          ],
          "source": [
            "obj-102",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-8",
            0
          ],
          "source": [
            "obj-11",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-38",
            0
          ],
          "source": [
            "obj-12",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-16",
            0
          ],
          "source": [
            "obj-14",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-13",
            0
          ],
          "source": [
            "obj-16",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-100",
            0
          ],
          "order": 0,
          "source": [
            "obj-2",
            6
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-101",
            0
          ],
          "order": 0,
          "source": [
            "obj-2",
            5
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-102",
            0
          ],
          "order": 0,
          "source": [
            "obj-2",
            3
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-20",
            0
          ],
          "source": [
            "obj-2",
            2
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-25",
            0
          ],
          "order": 1,
          "source": [
            "obj-2",
            3
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-28",
            0
          ],
          "source": [
            "obj-2",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-30",
            0
          ],
          "order": 1,
          "source": [
            "obj-2",
            5
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-5",
            0
          ],
          "order": 1,
          "source": [
            "obj-2",
            6
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-56",
            0
          ],
          "source": [
            "obj-2",
            4
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-6",
            0
          ],
          "source": [
            "obj-2",
            1
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-19",
            0
          ],
          "source": [
            "obj-20",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-11",
            0
          ],
          "source": [
            "obj-22",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-24",
            0
          ],
          "source": [
            "obj-25",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-27",
            0
          ],
          "source": [
            "obj-28",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-29",
            0
          ],
          "source": [
            "obj-30",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-2",
            2
          ],
          "source": [
            "obj-35",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-2",
            1
          ],
          "source": [
            "obj-37",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-35",
            0
          ],
          "source": [
            "obj-38",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-37",
            0
          ],
          "source": [
            "obj-40",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-4",
            0
          ],
          "source": [
            "obj-5",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-55",
            0
          ],
          "source": [
            "obj-56",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-3",
            0
          ],
          "source": [
            "obj-6",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-14",
            0
          ],
          "order": 0,
          "source": [
            "obj-8",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "obj-2",
            0
          ],
          "order": 1,
          "source": [
            "obj-8",
            0
          ]
        }
      }
    ],
    "dependency_cache": [
      {
        "name": "cusp~.mxo",
        "type": "iLaX"
      }
    ],
    "autosave": 0
  }
}
