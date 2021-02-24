import json
import sys
from jinja2 import FileSystemLoader, Environment
import xml.etree.ElementTree as ET

# Constants
TEMPLATE_FILE = "gtest_template.html"
OUTPUT_FILE = "gtest_output.html"

def process_input(input_file):
    """ Processes the input file.
        Will return a JSON object to be used by the HTML parser.
        If the file is in XML format it will be turned into a JSON object.
    """

    data = None

    with open(input_file) as gtest_json:
        if input_file.endswith('.json'):
            data = json.load(gtest_json)
        elif input_file.endswith('.xml'):
            # Need to turn the XML into the same format as the JSON
            data = process_xml(gtest_json)
        else:
            print("Unknown file type.")
            return

    return data

def process_xml(xml):
    """ Processes the XML file.
        Will return a JSON object that matches that created by GTEST.
    """

    tree = ET.parse(xml)
    root = tree.getroot()
    overviewName = root.attrib['name']
    overviewTests = int(root.attrib['tests'])
    overviewFailed = int(root.attrib['failures'])
    overviewDisabled = int(root.attrib['disabled'])
    data = {
        'name': overviewName,
        'tests': overviewTests,
        'failures': overviewFailed,
		'disabled': overviewDisabled,
        'testsuites': []
    }

    for child in root:
        testSuitename = child.attrib['name']
        totalTests = int(child.attrib['tests'])
        failed = int(child.attrib['failures'])
        disabled = int(child.attrib['disabled'])

        tempTest = []
        for test in child:
            testName = test.attrib['name']
            testTime = test.attrib['time']
            testStatus = test.attrib['status'].upper()
            # Getting all of the failure messages
            testFailures = []
            for failure in test:
                testFailure = failure.attrib['message']
                testFailures.append({
                    'failure': testFailure
                })

            # If there are no failures dont add it to the JSON
            if testFailures:
                tempTest.append({
                    'name': testName,
                    'time': testTime,
                    'status': testStatus,
                    'failures': testFailures
                })
            else:
                tempTest.append({
                    'name': testName,
                    'status': testStatus,
                    'time': testTime
                })
        print(tempTest)
        tempTestSuite = {
            'name': testSuitename,
            'tests': totalTests,
            'failures': failed,
            'disabled': disabled,
            'testsuite': tempTest
        }
        data['testsuites'].append(tempTestSuite)

    return data

def create_html(data):
    """ Turns the JSON object into a HTML file.
        Will grab the template and render it with our JSON object.
    """
    templateLoader = FileSystemLoader(searchpath="./")
    templateEnv = Environment(loader=templateLoader)
    template = templateEnv.get_template(TEMPLATE_FILE)

    with open(OUTPUT_FILE, "w") as output_html:
        output_html.write(template.render(test_overview=data, test_suites=data['testsuites']))

if __name__ == "__main__":
    json_data = process_input(sys.argv[1])
    create_html(json_data)
