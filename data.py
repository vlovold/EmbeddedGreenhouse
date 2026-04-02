
sensor_data = {
    "temperature": 25,
    "soil_humidity": 0.7,
    "air_humidity": 1,
    "brightness": 600,
    "fan_speed": 400
}

def usart_read(data, sensor):
    return data[sensor]
