## Welcome to GitHub Pages

You can use the [editor on GitHub](https://github.com/mariotalavera/plantstation/edit/master/README.md) to maintain and preview the content for your website in Markdown files.

Whenever you commit to this repository, GitHub Pages will run [Jekyll](https://jekyllrb.com/) to rebuild the pages in your site, from the content in your Markdown files.

### Markdown

Markdown is a lightweight and easy-to-use syntax for styling your writing. It includes conventions for

```markdown
Syntax highlighted code block

# Header 1
## Header 2
### Header 3

- Bulleted
- List

1. Numbered
2. List

**Bold** and _Italic_ and `Code` text

[Link](url) and ![Image](src)
```

For more details see [GitHub Flavored Markdown](https://guides.github.com/features/mastering-markdown/).

### Jekyll Themes

Your Pages site will use the layout and styles from the Jekyll theme you have selected in your [repository settings](https://github.com/mariotalavera/plantstation/settings). The name of this theme is saved in the Jekyll `_config.yml` configuration file.

### Support or Contact


# This is a dump on the current state of Plantstation2

## Break effort by main areas:
1. Hardware
 - Arduino Mega (main)
 - Iduino Yun Shield (wifi module+)
 - Connector shield (coming soon)  Will allow better sensor connections to computer.
 - Sensors
 	 - Have now
 	 - In near future
 	
2. Software
 - Arduino software in Mega
 - Ilos
 	 - Logstash
 	 - Zeppelin
 - Lucy
 	 - MySQL DB
 	 - MSSQL DB
 - AWS
 	 - API Gateway
 	 - DynamoDB
  
3. Data Flow - Everything goes thru Logstash
 - Inputs
 	-- **http poller** Wunderweather API, Current conditions.
 	-- **http** Arduino HTTP POST. 
    -- **jdbc**
 	   - grab last completed record from db (mysql) 
 - Filters
 	-- **json** 'message' source defined 
 	-- **mutate** Parsing Wunderweather json
 - Outputs
 	-- **jdbc** 
 	    - Insert arduino measurements into databases (MSSQL and MySQL Servers)
 	    - Cleanup of funny business 
 	    - Update db records with current weather conditions
 	-- **http** 
 	    - Insert into aws dynamodb

4. Monitoring with Zeppelin
 - Sensor dashboard
 	- Temperature, air (including current conditions) and water
 	- Heat index
 	- Humidity (arduino and local)
 	- Light %, value
 	- is led on? - figure out if its dark from this
 	- dew point
 	- uv index
 	- precipitation
 	- weather text
 - Event dashboard
 	- Location, app id.
 	- Events
 		- Total
 		- Daily
 		- Hourly

 

#### Notes
 - Mention Accuweather
 - have to fix pipe air temperature

Having trouble with Pages? Check out our [documentation](https://help.github.com/categories/github-pages-basics/) or [contact support](https://github.com/contact) and we’ll help you sort it out.
