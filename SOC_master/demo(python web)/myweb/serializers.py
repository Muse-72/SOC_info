from rest_framework import serializers
from .models import BatteryOutputList
from .models import BatteryInfo


class BOListModelSerializer(serializers.ModelSerializer):  # BatteryOutputList
    class Meta:
        model = BatteryOutputList
        fields = "__all__"


class BInfoModelSerializer(serializers.ModelSerializer): #BatteryInfo
    class Meta:
        model = BatteryInfo
        fields = "__all__"