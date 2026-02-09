import speedtest
import time
import datetime
import csv
import schedule
import winsound

# interval between data updates in minutes
INTERVAL = 10
# lowest allowed download and upload speed
LOWEST = (20,20)

def main():
    
    servers = []
    st = speedtest.Speedtest()

    st.get_servers(servers)
    st.get_best_server()
    print("Loaded")

    while True:
        schedule.every(INTERVAL).minutes.do(getData(st))

def getData(st: speedtest):

    # maybe add in a way to track the log files
    filename = 'log.csv'
    # clear csv file
    with open(filename, 'w', newline='') as f:
        pass

    # append data
    with open(filename, 'a', newline='') as csvFile:

        writer = csv.writer(csvFile)
        # header
        writer.writerow(['Timestamp', 'Download', 'Upload'])

        endTime = time.time() + (INTERVAL * 60)

        try:
            # let the loop run for the specified interval
            while time.time() < endTime:

                # get the speedtest data with timestamp
                dt = datetime.datetime.now()
                formatedDt = dt.strftime("%H:%M:%S")

                download = round(st.download() / 1000000, 1)
                upload = round(st.upload() / 1000000, 1)

                # if the download / upload speed is below a certain threshold, sound an alarm
                if (download < LOWEST[0] or upload < LOWEST[1]):
                    alarm()

                data = [formatedDt, download, upload]

                writer.writerow(data)
                csvFile.flush()

                print(data)

        except KeyboardInterrupt:
            print('Programme has been terminated by user.')    


def alarm():
    winsound.PlaySound("alarm.wav", winsound.SND_FILENAME | winsound.SND_ASYNC)


if __name__ == "__main__":
    main()