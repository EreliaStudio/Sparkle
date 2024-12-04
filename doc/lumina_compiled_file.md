# Concept
The idea behind Lumina is to abstract the GLSL from the user, using the langage as a sort of "script"
The compiler will than take the script as input, and compute a file that will contain all the information available about this shader

# Content
The content of the compiled file is human readable, and will be a set of "section", containing information about a certain aspect of the shader.

## Subdivision
All the section of the compiled file are presented like this : `## XXX ##`
With "XXX" been the section.

All the shader will contain XXX sections :
- LAYOUTS DEFINITION, describing the input of the shader
- FRAMEBUFFER DEFINITION, describing all the output of the shader, and all the framebuffer related information -> binding point, type and name
- EXCEPTION DEFINITION, describing all the possible exception raisable by the shader, linked to a specific layout buffer containing the structure corresponding to the parameters of the raiseExeception.
- CONSTANTS DEFINITION, describing all the ConstantBlock of the shader
- ATTRIBUTES DEFINITION, describing all the AttributeBlock of the shader
- TEXTURES DEFINITION, describing all the texture used by the shader, with the "descriptive" name and the "real" name used in the compiled GLSL
- VERTEX SHADER CODE, containing the GLSL code for the vertex pass
- FRAGMENT SHADER CODE, containing the GLSL code for the fragment pass 

## Layout definition
All layout must be describe following this pattern :
LayoutLocation Type Name

The order of the layouts must follow the LayoutLocation order, as it is an integer

## Frame buffer definition
All framebuffers must be describe following this pattern :
LayoutLocation Type Name

The order of the framebuffers must follow the LayoutLocation order, as it is an integer

## Exception definition
The exception must be store as a JSON, such as follow :
```
{
	"Exceptions":[
		{
			"Format":"MyErrorText with format : %d",
			"Parameter":[
				{
					"Name": "A",
					"Offset": 0,
					"Size": 4,
					"NestedElements": []
				}
			]
		},
		{
			"Format":"MyOtherErrorText with format : %v2 - %f",
			"Parameter":[
				{
					"Name": "A",
					"Offset": 0,
					"Size": 8,
					"NestedElements": [
						{ "Name": "X",  "Offset": 0,   "Size": 4  },
						{ "Name": "Y",  "Offset": 4,  "Size": 4  },]
				},
				{
					"Name": "B",
					"Offset": 0,
					"Size": 4,
					"NestedElements": []
				}
			]
		}
	]
}
```

## Constants and Attributes definition
The constants and the attribute sections contain a JSON describing the UBO and SSBO with their inner layout, such as follow :
### UBO
```
{
  "Name": "myUBO",
  "Binding": 0,
  "Size": 160,
  "NestedElements": [
    { "Name": "valueA",  "Offset": 0,   "Size": 64  },
    { "Name": "valueB",  "Offset": 64,  "Size": 64  },
    {
      "Name": "valueC",
      "Offset": 128,
      "Size": 32,
      "NestedElements": [
        { "Name": "valueCA", "Offset": 0,  "Size": 4  },
        { "Name": "valueCB", "Offset": 16, "Size": 16 }
      ]
    }
  ]
}
```
### SSBO
```
{
  "Name": "mySSBO",
  "Binding": 0,
  "NestedElements": [
    { "Name": "valueA",  "Offset":   0,  "Size": 64 },
    { "Name": "valueB",  "Offset":  64,  "Size": 64 },
  ],
  "DynamicArray":{
	"Name": "elements",
	"Offset": 128,
	"NbElements": 0,
	"ElementSize": 32,
	"ElementPadding": 0,
	"ElementComposition": [
	  { "Name": "valueEA", "Offset": 0,  "Size": 4  },
	  { "Name": "valueEB", "Offset": 16, "Size": 16 }
	]
  }
}
```

## Texture definition
Each texture used by the shader will be describe here.
As the GLSL will need a sort of "redefinition" of the texture name, we will store here the link between the name the user defined in Lumina and the name used in the shader, such as follow :
TextureGivenByUser TextureNameUsedByGLSL

## Vertex and Fragment code section
Each section will contain the GLSL code relative to the specific shader section